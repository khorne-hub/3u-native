// The renderer thread's look past the call it runs (mh3u-rend): the next display transfer its queue holds. A game that
// draws the touch screen only some frames (MH3U: one in five) never alternates its turns with the top screen's, so the
// renderer cannot tell from the order alone that the draws after a top screen's transfer are the hidden touch screen's
// (NativeGpu::NoteScreenTransfer); the next transfer queued, when it is the touch screen's, tells. Set by QueueGpu on its
// renderer thread; elsewhere (no renderer thread: the deterministic runner, a test) nothing is known.
#pragma once

#include <optional>

#include "mh4n/gpu.h"

namespace mh4n::gpu {

// A display transfer's command words that say which it is: its input's address (word 1, a virtual address) and its
// output's size (word 4: the width in the low 16 bits, the height in the high 16)
struct QueuedTransfer {
    u32 input_vaddr = 0;
    u32 output_size = 0;
};
using NextTransferFn = std::optional<QueuedTransfer> (*)(void* self);
struct TransferLookahead {
    NextTransferFn next = nullptr;
    void* self = nullptr;
};
inline thread_local TransferLookahead g_transfer_lookahead;

/// The next display transfer queued after the call this thread runs, or nothing (none queued yet, or no queue).
inline std::optional<QueuedTransfer> NextQueuedTransfer() {
    const TransferLookahead& l = g_transfer_lookahead;
    return l.next != nullptr ? l.next(l.self) : std::nullopt;
}

}  // namespace mh4n::gpu
