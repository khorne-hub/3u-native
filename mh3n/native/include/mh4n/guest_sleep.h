// The guest's SleepThread as the port may shape it. Header only, like pacing_trace.h: the kernel's SVC (svc.cpp)
// reads the pointers, the port (port/src/hooks.cpp) sets them.
//
// MH4U's job workers look for work in a loop that sleeps 1 ms between looks; at a frame rate above 60 the game's main
// thread waits on them every frame, and that millisecond (half of it on average) was a fifth of a 4 ms frame at "No
// limit". The port's job_poll_us option (off by default) shortens that one sleep.
//
// MH3U's main thread waits on polling loops of its own when it stops its local-wireless session (every 13 to 17 s,
// in towns and quests alike: the session is hosted again): it ends the session's two threads, each of which sleeps
// 16 ms between its looks at the stop flag, and then looks every 17 ms whether the session's work is done. On the
// console that is a frame or two at 30; here it was a frame of 40 to 60 ms about four times a minute. The two hooks
// below let the port end those waits as soon as what they wait for is there: a sleep that only paces a look at a flag
// ends when another thread starts to wait for its thread to end (the thread then looks, sees the flag and ends), and
// a look that waits for other threads' work comes round sooner. Real-time runs only: the deterministic mode keeps the
// reference's timeline (svc.cpp asks kernel_detail::Deterministic).
#pragma once

#include <cstdint>

namespace mh4n {

// Null: a guest thread sleeps the time it asked for. Otherwise the time to sleep for a SleepThread called from
// `return_address` (the caller's lr) that asked for `ns`. Called on the scheduler's thread, in every mode.
inline std::int64_t (*g_sleep_filter)(std::uint32_t return_address, std::int64_t ns) = nullptr;

// Null: as g_sleep_filter, but only in real time (the deterministic mode does not call it). Applied after it.
inline std::int64_t (*g_poll_sleep_filter)(std::uint32_t return_address, std::int64_t ns) = nullptr;

// Null: a thread that another thread waits to end (svcWaitSynchronization on its handle, with a timeout other than 0)
// sleeps its time out. Otherwise, when this says true for the return address of the SleepThread the thread is in, that
// sleep ends at once (as if its time were up). Real time only. Called on the scheduler's thread.
inline bool (*g_join_ends_sleep)(std::uint32_t sleep_return_address) = nullptr;

}  // namespace mh4n
