/*
 * Native test harness for recompiled code (Linux/macOS; Windows with llvm-mingw).
 *
 * Guest memory is a 4 GiB PROT_NONE reservation; the Python side commits and fills the
 * regions it needs. Every way generated code can leave a call (system call, trap, bad
 * return, exhausted budget, fault on unmapped/protected memory, unknown indirect target)
 * is turned into a status code via siglongjmp, so the caller can compare with Unicorn.
 * On Windows the reservation is VirtualAlloc's, a vectored exception handler takes the
 * faults (it moves the faulting thread to a landing function that jumps back), and the
 * jumps are __builtin_setjmp/__builtin_longjmp, which restore registers without unwinding.
 */
#define _GNU_SOURCE
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>   /* _resetstkoflw */
#else
#include <sys/mman.h>
#endif

#include "recomp.h"

extern const RFuncEntry rc_func_table[];
extern const u32 rc_func_count;
extern const u32 rc_hook_addrs[], rc_hook_count;

enum { ST_RETURN = 0, ST_SVC = 1, ST_FAULT = 2, ST_BUDGET = 3, ST_TRAP = 4, ST_BADRET = 5, ST_NOFUNC = 6 };

#define GUEST_SPAN ((size_t)1 << 32)
#define GUARD ((size_t)1 << 16)

static u8 *g_mem;
#if defined(_WIN32)
static void *g_jmp[5];                       /* __builtin_setjmp's buffer */
#define RH_SETJMP() __builtin_setjmp(g_jmp)
#define RH_LONGJMP() __builtin_longjmp(g_jmp, 1)
#else
static sigjmp_buf g_jmp;
#define RH_SETJMP() sigsetjmp(g_jmp, 1)
#define RH_LONGJMP() siglongjmp(g_jmp, 1)
#endif
static volatile sig_atomic_t g_in_call;
static volatile u32 g_status, g_info0, g_info1;
static volatile uintptr_t g_fault;
static int g_handlers;

#if defined(_WIN32)
int rh_init(void) {
    if (g_mem) return 0;
    void *p = VirtualAlloc(NULL, GUEST_SPAN + GUARD, MEM_RESERVE, PAGE_NOACCESS);
    if (!p) return -1;
    g_mem = (u8 *)p;
    return 0;
}

/* prot: 0 none, 1 read, 3 read/write (committed on first use; "none" keeps the pages, without access) */
int rh_map(u32 addr, u32 size, int prot) {
    DWORD p = prot == 0 ? PAGE_NOACCESS : prot == 1 ? PAGE_READONLY : PAGE_READWRITE;
    if (!VirtualAlloc(g_mem + addr, size, MEM_COMMIT, PAGE_READWRITE)) return -1;
    DWORD old;
    return VirtualProtect(g_mem + addr, size, p, &old) ? 0 : -1;
}
#else
int rh_init(void) {
    if (g_mem) return 0;
    void *p = mmap(NULL, GUEST_SPAN + GUARD, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) return -1;
    g_mem = (u8 *)p;
    return 0;
}

/* prot: 0 none, 1 read, 3 read/write */
int rh_map(u32 addr, u32 size, int prot) {
    int p = prot == 0 ? PROT_NONE : prot == 1 ? PROT_READ : PROT_READ | PROT_WRITE;
    return mprotect(g_mem + addr, size, p);
}
#endif
void rh_write(u32 addr, const void *src, u32 n) { memcpy(g_mem + addr, src, n); }
void rh_read(u32 addr, void *dst, u32 n) { memcpy(dst, g_mem + addr, n); }
u32 rh_info0(void) { return g_info0; }
u32 rh_info1(void) { return g_info1; }
u32 rh_fault_addr(void) { return (u32)(g_fault - (uintptr_t)g_mem); }
u32 rh_func_count(void) { return rc_func_count; }
void rh_func_at(u32 i, u32 *addr, u32 *mode) { *addr = rc_func_table[i].addr; *mode = rc_func_table[i].mode; }
/* install (or with NULL remove) a hook on a hook point; 0 if addr isn't one */
int rh_set_hook(u32 addr, RHookFn hook) {
    for (u32 i = 0; i < rc_hook_count; i++) {
        if (rc_hook_addrs[i] == addr) {
            rc_hooks[i] = hook;
            return 1;
        }
    }
    return 0;
}

static const RFuncEntry *find(u32 addr) {
    u32 lo = 0, hi = rc_func_count;
    while (lo < hi) {
        u32 mid = (lo + hi) / 2;
        if (rc_func_table[mid].addr < addr) lo = mid + 1;
        else hi = mid;
    }
    if (lo < rc_func_count && rc_func_table[lo].addr == addr) return &rc_func_table[lo];
    return NULL;
}

/* Yield points. RC_NATIVE builds look at *rc_preempt_flag (the harness owns that word and
 * rh_set_preempt_flag sets it); the other builds count cycles in ctx->budget. Either
 * way rc_budget ends the call with ST_BUDGET, unless rh_set_budget_resume(refill) with
 * refill >= 0 makes it behave like a host that yields and comes back: it clears the flag,
 * adds `refill` cycles to ctx->budget and returns. rh_budget_calls() counts the calls of
 * rc_budget made by the latest rh_call. */
static volatile u32 g_preempt;
volatile u32 *rc_preempt_flag = &g_preempt;
static s64 g_resume_refill = -1;
static u32 g_budget_calls;
void rh_set_preempt_flag(u32 v) { g_preempt = v; }
u32 rh_preempt_flag(void) { return g_preempt; }
void rh_set_budget_resume(s64 refill) { g_resume_refill = refill; }
u32 rh_budget_calls(void) { return g_budget_calls; }

static void leave(u32 status, u32 i0, u32 i1) {
    g_status = status;
    g_info0 = i0;
    g_info1 = i1;
    RH_LONGJMP();
}

void rc_call(RCtx *ctx, u32 target) {
    const RFuncEntry *e = find(target & ~1u);
    if (!e || e->mode != (target & 1)) leave(ST_NOFUNC, target, 0);
    e->fn(ctx);
}
void rc_call_ic(RCtx *ctx, u32 target, u32 *ic_target, void (**ic_fn)(RCtx *)) {
    const RFuncEntry *e = find(target & ~1u);
    if (!e || e->mode != (target & 1)) leave(ST_NOFUNC, target, 0);
    *ic_target = target;
    *ic_fn = e->fn;
    e->fn(ctx);
}
void rc_svc(RCtx *ctx, u32 imm, u32 next_pc) { (void)ctx; leave(ST_SVC, imm, next_pc & ~1u); }
void rc_trap(RCtx *ctx, u32 pc, u32 kind) { (void)ctx; leave(ST_TRAP, pc, kind); }
void rc_bad_return(RCtx *ctx, u32 got, u32 expected) { (void)ctx; leave(ST_BADRET, got, expected); }
void rc_budget(RCtx *ctx) {
    g_budget_calls++;
    if (g_resume_refill < 0) leave(ST_BUDGET, 0, 0);
    g_preempt = 0;
    ctx->budget += g_resume_refill;
}

#if defined(_WIN32)
/* The faulting thread continues here (the handler points its RIP at it), on its own stack. A host stack overflow
 * (a deep guest recursion) is a fault too, as the Linux harness's signal handler on its alternate stack sees it. */
static volatile int g_stack_overflow;
static void fault_landing(void) { RH_LONGJMP(); }

static LONG CALLBACK on_exception(EXCEPTION_POINTERS *e) {
    const DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_STACK_OVERFLOW && g_in_call) g_stack_overflow = 1;  /* (rh_call re-arms the guard page) */
    int sig = code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR || code == EXCEPTION_STACK_OVERFLOW
                  ? 11 /* SIGSEGV */
            : code == EXCEPTION_ILLEGAL_INSTRUCTION || code == EXCEPTION_PRIV_INSTRUCTION ? 4 /* SIGILL */
            : code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW ? 8 /* SIGFPE */
            : 0;
    if (!g_in_call || !sig) return EXCEPTION_CONTINUE_SEARCH;
    g_fault = sig == 11 && e->ExceptionRecord->NumberParameters >= 2
                  ? (uintptr_t)e->ExceptionRecord->ExceptionInformation[1] : 0;
    g_status = ST_FAULT;
    g_info0 = (u32)sig;
    e->ContextRecord->Rsp = (e->ContextRecord->Rsp & ~(DWORD64)15) - 8;   /* as if called */
    e->ContextRecord->Rip = (DWORD64)(uintptr_t)fault_landing;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void install(void) {
    if (g_handlers) return;
    AddVectoredExceptionHandler(1, on_exception);
    g_handlers = 1;
}
#else
static void on_signal(int sig, siginfo_t *si, void *uc) {
    (void)uc;
    if (!g_in_call) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }
    g_fault = (uintptr_t)si->si_addr;
    g_status = ST_FAULT;
    g_info0 = (u32)sig;
    RH_LONGJMP();
}

static void install(void) {
    if (g_handlers) return;
    static u8 altstack[1 << 16];
    stack_t ss = {.ss_sp = altstack, .ss_size = sizeof altstack, .ss_flags = 0};
    sigaltstack(&ss, NULL);
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    g_handlers = 1;
}
#endif

/* Call the function at guest address `addr` (bit 0 = Thumb). Returns a status code. */
static int g_check_return = 1;
void rh_set_check_return(int on) { g_check_return = on; }

int rh_call(u32 addr, RCtx *ctx) {
    const RFuncEntry *e = find(addr & ~1u);
    if (!e) return ST_NOFUNC;
    install();
    ctx->mem = g_mem;
    g_status = ST_RETURN;
    g_info0 = g_info1 = 0;
    g_budget_calls = 0;
    const u32 expect = ctx->r[14];
    rc_apply_rounding(ctx->fpscr);          /* the host follows FPSCR.RMode in guest code */
    if (RH_SETJMP() == 0) {
        g_in_call = 1;
        e->fn(ctx);
        /* a tail-called function returns through its callee: check the final target */
        if (g_check_return && (ctx->r[15] & ~1u) != (expect & ~1u)) {
            g_status = ST_BADRET;
            g_info0 = ctx->r[15];
            g_info1 = expect;
        }
    }
    g_in_call = 0;
#if defined(_WIN32)
    if (g_stack_overflow) {
        g_stack_overflow = 0;
        _resetstkoflw();
    }
#endif
    rc_apply_rounding(0);
    return (int)g_status;
}
