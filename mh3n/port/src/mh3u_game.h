// MH3U (mh3u.h): reading the game's memory, and the game's structures as the port knows them (mh3u_game.cpp).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "mh3u.h"

namespace Port::Mh3u {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

void Log(const std::string& line);
const Settings& CurrentSettings();

/// Guest memory for one frame's reads: an address is read only where it is mapped (a page's answer is kept for the
/// frame), so a stale pointer reads as 0 instead of faulting.
class Guest {
public:
    explicit Guest(const Memory& m) : m_(m) {}
    bool Ok(u32 a, u32 n = 4) const {
        if (a < 0x00100000u || a + n < a) return false;
        const u32 first = a >> 12, last = (a + n - 1) >> 12;
        // (most reads are on the page of the read before: its answer without a lookup in the frame's table)
        if (first == last && first == last_page_) return last_ok_;
        for (u32 p = first; p <= last; ++p) {
            auto it = pages_.find(p);
            if (it == pages_.end()) {
                const bool mapped = m_.mapped ? m_.mapped(m_.user, p << 12, 0x1000) : false;
                it = pages_.emplace(p, mapped).first;
            }
            last_page_ = p, last_ok_ = it->second;
            if (!it->second) return false;
        }
        return true;
    }
    u32 R32(u32 a) const {
        u32 v = 0;
        if ((a & 3) == 0 && Ok(a, 4)) std::memcpy(&v, m_.mem + a, 4);
        return v;
    }
    u16 R16(u32 a) const {
        u16 v = 0;
        if ((a & 1) == 0 && Ok(a, 2)) std::memcpy(&v, m_.mem + a, 2);
        return v;
    }
    u8 R8(u32 a) const { return Ok(a, 1) ? m_.mem[a] : 0; }
    float F32(u32 a) const {
        const u32 v = R32(a);
        float f;
        std::memcpy(&f, &v, 4);
        return f;
    }
    bool W32(u32 a, u32 v) const {
        if ((a & 3) != 0 || !Ok(a, 4)) return false;
        std::memcpy(m_.mem + a, &v, 4);
        return true;
    }
    bool WF32(u32 a, float f) const {
        u32 v;
        std::memcpy(&v, &f, 4);
        return W32(a, v);
    }
    /// A pointer to a heap object: the game's objects live on its heap (0x08000000 up) and on the linear heap
    /// (0x14000000 up, 0x30000000 up on the newer kernels: MH3U puts its areas and the hunter's unit there, mh3u-caza)
    static bool Heap(u32 p) {
        return (p & 3) == 0 && ((p >= 0x08000000u && p < 0x10000000u) || (p >= 0x14000000u && p < 0x1c000000u) ||
                                (p >= 0x30000000u && p < 0x38000000u));
    }

private:
    Memory m_;
    mutable std::unordered_map<u32, bool> pages_;
    mutable u32 last_page_ = ~0u;  // (the last page looked up, and its answer: Ok's fast path)
    mutable bool last_ok_ = false;
};

/// An interface unit alive this frame (sUnit's lines): its address, class and cUnit flags (bits 16-25 its viewports).
struct UnitView {
    u32 address;
    const char* name;
    u32 flags;
};

namespace Hud {
void Reset();
/// A HUD part's corner (Hooks::HudPartKey's order), a fraction of the top screen along `axis` (0 x, 1 y).
float PartAnchor(int part, int axis);
const float* PartRect(int part);
bool InQuest();  // (the last frame's step was in a quest)
bool Bow();      // (... with the bow: SetBow, after the hunter's weapon is known)
void SetBow(bool bow);
/// After the units are known: the HUD's parts as the options say (hud_size, hud_<part>), and the app's view of them in
/// s.hud. `quest`: the hunter is in a quest (the parts are shown only there).
void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, bool quest,
          Hooks::GameState& s);
}  // namespace Hud

namespace Panels {
void Reset();
/// After the units and the touch menu are known: the quest's Item Pouch panel's slot for the app's quick items
/// (s.items_slot, s.touch_items_slot), mh3u_panels.cpp.
void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, bool quest,
          Hooks::GameState& s);
}  // namespace Panels

namespace OneScreen {
void Reset();
/// A unit's flags as the game has them: its viewport bits before the single screen gave it the top screen's.
u32 GameFlags(u32 unit, u32 flags);
/// After the touch menu is known (GameState touch_menu, touch_menu_place `place`): its units (`menu_units`, indices in
/// `units`) drawn on the top screen at their places while the single screen is on and the touch screen not shown, the
/// others given back (mh3u_onescreen.cpp); s.touch_menu_top says whether any is. `tavern`: Port Tanzia's Tavern
/// (aHarborGuild), where the town panel's frame is never a menu of its own.
void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, u32 top_views,
          const std::vector<std::size_t>& menu_units, int place, bool tavern, Hooks::GameState& s);
/// (research) mh3u_one's value: see mh3u_onescreen.cpp.
bool SetOption(const std::string& value);
}  // namespace OneScreen

namespace Game {
/// A new boot: nothing carries over.
void Reset();
/// One frame: the state for the app (and the options applied to the game's memory).
Hooks::GameState Step(const Guest& g, u32 frame, const Settings& settings);
/// For the log: what the frame saw (the interface units on each screen).
std::string Describe();
/// (research) mh3u_probe's value: see mh3u_game.cpp.
bool SetProbe(const std::string& value);
}  // namespace Game

}  // namespace Port::Mh3u
