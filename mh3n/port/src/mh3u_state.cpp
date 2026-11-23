// MH3U: what the game is doing, from its memory once a frame (mh3u.h). No hook point: the runtime's frame watcher calls
// Frame at a safe point after each game frame, and everything here reads (or writes a few bytes of) guest memory.
#include "mh3u.h"

#include "mh3u_game.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include <fmt/format.h>

extern "C" {
#include "recomp.h"  // (RCodeInfo)
}

namespace Port::Mh3u {

namespace {

std::atomic<bool> active{false};
void (*log_line)(const std::string&) = nullptr;
std::mutex state_mutex;
Hooks::GameState state;         // State(), under the mutex
Hooks::GameState state_logged;  // trace_state: the last one logged
Settings settings;              // (the emulation thread's: Configure and Frame run there)
std::mutex settings_mutex;
Settings settings_next;         // Configure's, taken by the next Frame
bool settings_changed = false;

/// (a measurement aid) MH4N_MH3U_COST=1: what Frame costs, a line every 3600 frames (the average and the longest, in
/// microseconds), as hunt aids' MH4N_HUNT_COST
struct CostMeter {
    static bool On() {
        static const bool on = [] {
            const char* v = std::getenv("MH4N_MH3U_COST");
            return v != nullptr && *v != 0 && *v != '0';
        }();
        return on;
    }
    std::chrono::steady_clock::time_point start = On() ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
    ~CostMeter() {
        if (!On()) return;
        static std::int64_t total = 0, longest = 0;
        static std::uint32_t count = 0;
        const std::int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
        total += ns, longest = std::max(longest, ns);
        if (++count == 3600) {
            if (log_line) {
                log_line(fmt::format("mh3u: a frame's step took {:.2f} us on average, {:.2f} us at most, over {} frames",
                                     total / 1000.0 / count, longest / 1000.0, count));
            }
            total = longest = 0, count = 0;
        }
    }
};

}  // namespace

void Log(const std::string& line) {
    if (log_line) log_line("mh3u: " + line);
}

bool Active() {
    return active.load(std::memory_order_relaxed);
}

void SetLog(void (*log)(const std::string& line)) {
    log_line = log;
}

void Install(const RCodeInfo& info, std::vector<std::string>& log) {
    const bool on = info.title_id == kTitle;
    active = on;
    {
        std::lock_guard lock(state_mutex);
        state = state_logged = Hooks::GameState{};
    }
    Game::Reset();
    if (on) {
        log.push_back("MH3U: the game's state, the single screen and the HUD from its memory once a frame (mh3u_*.cpp)");
    }
}

void Configure(const Settings& s) {
    std::lock_guard lock(settings_mutex);
    settings_next = s;
    settings_changed = true;
}

// (research) mh3u_dump = <file> <hex address> <hex size>: that much of guest memory written to the file at the next
// frame (a session driven by hand: MH4N_SET_LIVE)
std::string dump_file;
u32 dump_address = 0, dump_size = 0;

bool SetOption(const std::string& key, const std::string& value) {
    if (key == "mh3u_probe") {
        std::lock_guard lock(settings_mutex);
        return Game::SetProbe(value);
    }
    if (key == "mh3u_sign") return SetSignProbe(value);
    if (key == "mh3u_one") return OneScreen::SetOption(value);
    if (key == "mh3u_dump") {
        char file[260] = {};
        unsigned a = 0, n = 0;
        if (std::sscanf(value.c_str(), "%259s %x %x", file, &a, &n) != 3) return false;
        std::lock_guard lock(settings_mutex);
        dump_file = file, dump_address = a, dump_size = n;
        return true;
    }
    return false;
}

const Settings& CurrentSettings() {
    return settings;
}

void Frame(const Memory& memory, std::uint32_t frame) {
    if (!Active() || memory.mem == nullptr) return;
    CostMeter cost;
    {
        std::lock_guard lock(settings_mutex);
        if (settings_changed) settings = settings_next, settings_changed = false;
    }
    Guest g{memory};
    {
        std::string file;
        u32 a = 0, n = 0;
        {
            std::lock_guard lock(settings_mutex);
            file.swap(dump_file), a = dump_address, n = dump_size;
        }
        if (!file.empty()) {
            std::FILE* f = std::fopen(file.c_str(), "wb");
            u32 written = 0;
            for (u32 p = a; f && p < a + n; p += 0x1000) {  // (a page not mapped: zeros)
                static const u8 zeros[0x1000] = {};
                const u32 len = std::min<u32>(0x1000, a + n - p);
                std::fwrite(g.Ok(p, len) ? memory.mem + p : zeros, 1, len, f);
                written += len;
            }
            if (f) std::fclose(f);
            Log(fmt::format("dump: frame {} {:08x}+{:x} to {} ({} bytes)", frame, a, n, file, written));
        }
    }
    Hooks::GameState s = Game::Step(g, frame, settings);
    if (settings.trace_state && (s.place != state_logged.place || s.menu != state_logged.menu ||
                                 s.front_end != state_logged.front_end || s.weapon != state_logged.weapon ||
                                 s.weapon_drawn != state_logged.weapon_drawn || s.ranged != state_logged.ranged ||
                                 s.touch_menu != state_logged.touch_menu || s.underwater != state_logged.underwater ||
                                 s.item_bar_shown != state_logged.item_bar_shown || s.town_panel != state_logged.town_panel ||
                                 s.items_slot != state_logged.items_slot)) {
        constexpr const char* places[] = {"other", "town", "quest"};
        Log(fmt::format("state: frame {} place {}{} menu {} touch menu {} (place {}) town panel {} weapon {} drawn {} "
                        "ranged {} underwater {} item bar {} quick items {} ({})",
                        frame, s.place < 0 ? "-" : places[s.place], s.front_end == 1 ? " (the front end)" : "", s.menu,
                        s.touch_menu, s.touch_menu_place, s.town_panel, s.weapon, s.weapon_drawn, s.ranged, s.underwater,
                        s.item_bar_shown, s.items_slot, Game::Describe()));
        state_logged = s;
    }
    std::lock_guard lock(state_mutex);
    state = s;
}

Hooks::GameState State() {
    std::lock_guard lock(state_mutex);
    return state;
}

}  // namespace Port::Mh3u
