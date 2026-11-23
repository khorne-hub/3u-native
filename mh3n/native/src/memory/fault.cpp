// Bad guest accesses (N1-02): the report, the table of reservations and the guest thread context.
//
// Everything reachable from HandleFault is async-signal-safe: loads from fixed arrays and
// thread-local PODs with constant initialisation, write(2), the host's FatalHook (which must be
// async-signal-safe too) and _exit. No fmt, no allocation, no locks, no stdio.
#include <atomic>

#if defined(_WIN32)
#include <io.h>
#include <process.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdlib>
#else
#include <unistd.h>
#endif

#include "host.h"

namespace mh4n {
namespace {

struct FaultContext {
    const u32* regs; // an RCtx; r[14] is the LR
    u32 thread_id;
};
thread_local FaultContext t_context = {nullptr, 0};
std::atomic<FatalHook> g_fatal_hook{nullptr};

class Writer {
public:
    Writer(char* out, std::size_t capacity) : out_(out), capacity_(capacity) {}
    void Str(const char* s) {
        while (*s != '\0') {
            Put(*s++);
        }
    }
    void Hex32(u32 value) {
        Str("0x");
        for (int shift = 28; shift >= 0; shift -= 4) {
            Put("0123456789abcdef"[(value >> shift) & 0xF]);
        }
    }
    void Dec(u32 value) {
        char digits[10];
        int n = 0;
        do {
            digits[n++] = static_cast<char>('0' + value % 10);
            value /= 10;
        } while (value != 0);
        while (n > 0) {
            Put(digits[--n]);
        }
    }
    std::size_t size() const { return size_; }

private:
    void Put(char c) {
        if (size_ < capacity_) {
            out_[size_] = c;
        }
        ++size_;
    }
    char* out_;
    std::size_t capacity_;
    std::size_t size_ = 0;
};

#if defined(_WIN32)
// A research aid (Windows only): with MH4N_FAULT_DUMP_DIR set, a bad guest access also writes the guest's memory as it
// is at the fault into that folder (one file per range, guest_<start>.bin; pages not committed as zeros), so the
// state that led to it can be read. Never set for players.
void DumpGuestMemory(std::uintptr_t base) {
    const char* dir = std::getenv("MH4N_FAULT_DUMP_DIR");
    if (dir == nullptr || *dir == '\0') return;
    static const u32 kRanges[][2] = {{0x00100000, 0x01200000}, {0x08000000, 0x0E000000}, {0x0FF00000, 0x10000000},
                                     {0x14000000, 0x1C000000}, {0x1F000000, 0x1F600000}, {0x1FF00000, 0x1FF80000}};
    static char zeros[0x1000];
    for (const auto& range : kRanges) {
        char path[512];
        Writer name(path, sizeof path - 1);
        name.Str(dir);
        name.Str("/guest_");
        name.Hex32(range[0]);
        name.Str(".bin");
        path[name.size() < sizeof path - 1 ? name.size() : sizeof path - 1] = '\0';
        HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) continue;
        for (u32 a = range[0]; a < range[1]; a += 0x1000) {
            const void* page = reinterpret_cast<const void*>(base + a);
            MEMORY_BASIC_INFORMATION info{};
            const bool readable = VirtualQuery(page, &info, sizeof info) == sizeof info && info.State == MEM_COMMIT &&
                                  (info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ |
                                                   PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY)) != 0 &&
                                  (info.Protect & PAGE_GUARD) == 0;
            DWORD written = 0;
            WriteFile(file, readable ? page : zeros, 0x1000, &written, nullptr);
        }
        CloseHandle(file);
    }
}
#endif

} // namespace

void SetFaultContext(const void* rctx, u32 thread_id) {
    t_context = {static_cast<const u32*>(rctx), thread_id};
}

void ClearFaultContext() {
    t_context = {nullptr, 0};
}

void SetFatalHook(FatalHook hook) {
    g_fatal_hook.store(hook);
}

std::size_t FormatFaultReport(char* out, std::size_t capacity, u32 guest_addr, FaultAccess access,
                              bool has_context, u32 thread_id, u32 lr) {
    Writer w(out, capacity);
    w.Str("mh4n: bad guest memory access: ");
    w.Str(access == FaultAccess::Read ? "read" : access == FaultAccess::Write ? "write" : "access");
    w.Str(" at ");
    w.Hex32(guest_addr);
    if (has_context) {
        w.Str(" (guest thread ");
        w.Dec(thread_id);
        w.Str(", lr ");
        w.Hex32(lr);
        w.Str(")\n");
    } else {
        w.Str(" (no guest thread)\n");
    }
    return w.size() < capacity ? w.size() : capacity;
}

} // namespace mh4n

namespace mh4n::fault {
namespace {

constexpr std::size_t kMaxReservations = 16;
struct Slot {
    std::atomic<std::uintptr_t> base{0};
    std::atomic<std::uintptr_t> end{0};
};
Slot g_slots[kMaxReservations];

} // namespace

bool RegisterReservation(std::uintptr_t base, std::size_t size) {
    for (Slot& slot : g_slots) {
        std::uintptr_t expected = 0;
        // a slot whose end is still 0 matches nothing
        if (slot.base.compare_exchange_strong(expected, base)) {
            slot.end.store(base + size);
            return true;
        }
    }
    return false;
}

void UnregisterReservation(std::uintptr_t base) {
    for (Slot& slot : g_slots) {
        if (slot.base.load() == base) {
            slot.end.store(0);
            slot.base.store(0);
            return;
        }
    }
}

bool HandleFault(std::uintptr_t host_addr, FaultAccess access) {
    for (const Slot& slot : g_slots) {
        const std::uintptr_t base = slot.base.load();
        if (base == 0 || host_addr < base || host_addr >= slot.end.load()) {
            continue;
        }
        // A fault in the guard after the 4 GiB is reported as the address it would have been
        // (wrapped to 32 bits).
        const u32 guest_addr = static_cast<u32>(host_addr - base);
        char line[160];
        const FaultContext& ctx = t_context;
        const std::size_t n =
            FormatFaultReport(line, sizeof line, guest_addr, access, ctx.regs != nullptr,
                              ctx.thread_id, ctx.regs != nullptr ? ctx.regs[14] : 0);
#if defined(_WIN32)
        (void)!_write(2, line, static_cast<unsigned>(n));
#else
        (void)!write(2, line, n);
#endif
        if (ctx.regs != nullptr) {
            // the guest registers as the context last held them (generated code keeps them in locals between calls,
            // so these are the values at the last call, system call or yield): where to look in the game's code
            char regs[400];
            Writer r(regs, sizeof regs);
            r.Str("mh4n:   registers at the last call boundary:");
            for (u32 i = 0; i < 16; ++i) {
                r.Str(i == 13 ? " sp=" : i == 14 ? " lr=" : i == 15 ? " pc=" : i < 10 ? " r" : " r1");
                if (i < 13) {
                    const char digit[2] = {static_cast<char>('0' + (i < 10 ? i : i - 10)), '\0'};
                    r.Str(digit);
                    r.Str("=");
                }
                r.Hex32(ctx.regs[i]);
            }
            r.Str("\n");
            const std::size_t m = r.size() < sizeof regs ? r.size() : sizeof regs;
#if defined(_WIN32)
            (void)!_write(2, regs, static_cast<unsigned>(m));
#else
            (void)!write(2, regs, m);
#endif
        }
#if defined(_WIN32)
        DumpGuestMemory(base);
#endif
        if (const FatalHook hook = g_fatal_hook.load()) {
            hook();  // (the host's error screen, before the process is gone)
        }
        _exit(3);
    }
    return false;
}

} // namespace mh4n::fault
