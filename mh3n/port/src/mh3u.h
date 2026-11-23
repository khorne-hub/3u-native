// MH3U (Monster Hunter 3 Ultimate, 00040000000AE400): the port's features on that game's own code. MH4U's port code
// (hooks.cpp, onescreen.cpp...) is left as it is; with an MH3U code library hooks.cpp hands over to these from its own
// entry points (Install, SetOption, CurrentGameState), and the platform calls Frame once a game frame. Nothing here runs
// with any other game's library, so MH4U's behaviour and its library id are unchanged.
//
// What the game is doing (mh3u_state.cpp) comes from its memory once a frame, with no hook point: the area on top
// (sArea), the game's interface units (sUnit's lines: the uID* units, each drawn on the screens its view mask says), the
// local hunter (sPlayer) and its HUD (uIDCockpit*). docs/MH3U.md lists every address with how it was found.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hooks.h"

struct RCodeInfo;

namespace Port::Mh3u {

constexpr std::uint64_t kTitle = 0x00040000000AE400ull;

/// The code library loaded last is MH3U's (Install saw its program id). Any thread.
bool Active();

/// Where the log lines go (hooks.cpp's log).
void SetLog(void (*log)(const std::string& line));

/// From Hooks::Install, before anything of MH4U's: notes whether the library is MH3U's and resets the state. Lines for
/// the log go to `log`.
void Install(const RCodeInfo& info, std::vector<std::string>& log);

/// The options of mh4u.ini these use (hooks.cpp keeps the file's; it passes them at Install and at each change).
struct Settings {
    bool single_screen = false;
    bool quick_items = false;
    int trace_state = 0;     // trace_state: 1 the state as it changes, 2 also the interface units shown
    float hud_size = 1.0f;   // 0.6 .. 1
    float hud_dx[Hooks::HUD_PART_COUNT] = {}, hud_dy[Hooks::HUD_PART_COUNT] = {};
    float hud_k[Hooks::HUD_PART_COUNT] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    bool hud_hidden[Hooks::HUD_PART_COUNT] = {};
};
void Configure(const Settings& settings);

/// The options of MH3U's own that change while the game runs (the runner's --set: mh3u_probe, research). False for any
/// other key. Any thread: taken at the next frame.
bool SetOption(const std::string& key, const std::string& value);

/// Guest memory as the platform has it: `mem` the host address of guest address 0 (flat memory), `mapped(user, address,
/// size)` whether a range is mapped.
struct Memory {
    std::uint8_t* mem = nullptr;
    bool (*mapped)(void* user, std::uint32_t address, std::uint32_t size) = nullptr;
    void* user = nullptr;
};

/// Once a game frame, on the emulation thread (the runtime's frame watcher, at a safe point after the frame counter
/// moved): works out the game's state and applies the options that act on the game's memory.
void Frame(const Memory& memory, std::uint32_t frame);

/// (the uGUI draw hook, mh3u_gfx.inc) The HUD part a unit about to be drawn belongs to (by its vtable): its size, its move
/// and its corner, in top-screen pixels (400 x 240), and whether it is hidden (not drawn). False when the unit is no part
/// of the HUD or is drawn as the game draws it. On the emulation thread.
bool HudDraw(std::uint32_t vtable, float* size, float* dx, float* dy, float* anchor_x, float* anchor_y, bool* hidden);
/// As HudDraw, with the unit and its GUI's place (its instance's root matrix, + 0xf4 then + 0x10: the last row's x and y;
/// `placed` false without an instance; 0, 0 for every HUD unit seen so far). With trace_state each top-screen unit of a
/// quest is logged once (its class, its place, its part).
bool HudGuiDraw(std::uint32_t unit, std::uint32_t vtable, bool placed, float tx, float ty, float* size, float* dx, float* dy,
                float* anchor_x, float* anchor_y, bool* hidden);
/// A quest with the bow (the hunter's weapon; its own HUD unit, uIDCockpitBow, is not made): its coatings' panel at the
/// bottom right is then drawn by the base unit, uIDCockpit (mh3u-pc2's run a2: hiding uIDCockpit hid that panel and
/// nothing else). Any thread.
bool HudBow();
/// Whether the hunting HUD is up: a quest's top screen (the town and the front end have none); (mh3u-paridad) there the
/// HUD's sprites are drawn through the main view's projection (mh3u_gfx.inc). Any thread.
bool HudInQuest();

/// (research, mh3u_sign = <part> <side> [<names> <hidden> <dump>]) the maps' signs (uIDCockpitSign) with the area map's
/// part or not, at the map's side of the wide top screen or centred (both on by default); the unit's nodes that are the
/// hunters' names (a hex mask, bit i node i), nodes not drawn on the top screen (hex), and a dump of its nodes once (1)
bool SetSignProbe(const std::string& value);
/// (the uGUI draw hook) whether the maps' signs go to the area map's side of the wide top screen (hud_edges); never
/// while the unit's names are drawn (SetSignPass 1: the bars' side)
bool SignAtMapSide();
/// The sign unit's nodes (its + 0x1f8, 36 pointers) that are the hunters' names on their plates over the bars, as a mask
/// (bit i: node i; 0 the unit drawn whole with the map's part); the nodes not drawn (research); a dump asked (once).
/// The names are nodes 28 to 35 (their class's vtable 0x00b5739c, the others' 0x00b5a5a4 and 0x00b5cf00; node 31 the
/// player's name at (-43, 34): hiding it alone hid "Hunter" from its plate, mh3u-pc3's run g1); node 14 is the hunter's
/// arrow (turned with the hunter, placed from the map's block).
constexpr std::uint64_t kSignNameNodes = 0xff0000000ull;
std::uint64_t SignNameNodes();
std::uint64_t SignHiddenNodes();
bool SignDumpTake();
/// (the uGUI draw hook) the sign unit's draw going on: 0 the whole unit, 1 its names (the bars' part and side), 2 the rest
/// (the map's)
void SetSignPass(int pass);
bool SignNamePass();

/// (the HUD sprites' draw hook, mh3u_gfx.inc) As HudDraw, for a sprite of the HUD's (uDrawIDSprite: the clock and the
/// gauges) in a quest, by its drawn elements' place on the top screen (x, y: 400 x 240, from the top left; `placed`
/// false: none of its elements told).
bool HudSprite(bool placed, float x, float y, float* size, float* dx, float* dy, float* anchor_x, float* anchor_y,
               bool* hidden);
/// (trace_state) a HUD sprite seen on the top screen: logged once per unit (its elements' box: centre, size; their count)
void HudSpriteSeen(std::uint32_t unit, bool placed, float x, float y, float w, float h, std::uint32_t elements, bool part);

/// (the uGUI draw hook, mh3u_gfx.inc) A unit of the touch screen's that the single screen draws on the top screen this
/// frame (mh3u_onescreen.cpp): its size and move, p (the touch screen's pixels, 320 x 240) to size * p + (dx, dy) in the
/// top screen's 400 x 240. False for any other unit. On the emulation thread.
bool OneScreenDraw(std::uint32_t unit, float* size, float* dx, float* dy);
/// (the uGUI draw hook) A touch part the single screen draws on the top screen: of a unit the game draws on both screens
/// whose GUI nodes say which screen they are for (its flags + 0x158 bit 3, a node's screens + 0x78), the nodes for the
/// touch screen alone (its menu's help line, the item's panel with the quantity, a list's title), drawn again on the top
/// screen as on the touch screen, p to size * p + (dx, dy) as OneScreenDraw's. `node`: which of them (their order in the
/// tree, from 0), -1 all of them.
struct OneScreenPart {
    int node = -1;
    float size = 1.0f, dx = 0.0f, dy = 0.0f;
};
/// The touch parts of `unit` this frame (at most `max`), 0 for none. On the emulation thread.
int OneScreenParts(std::uint32_t unit, OneScreenPart* parts, int max);
/// (the app, each frame) Whether the touch screen shows now (its corner, stacked, side by side, the second display, the
/// town's large screen): the single screen then leaves the game's menus on it, as on the console. Any thread.
void SetTouchScreenShown(bool shown);

/// The state for Hooks::CurrentGameState. Any thread.
Hooks::GameState State();

// MH3U's local-wireless session as it stops (every 13 to 17 s, in towns and quests: it is hosted again), for the runtime's
// real-time hooks of guest_sleep.h (mh3u-rend2): true for the sleeps of the session's two threads between their looks at
// their stop flag (they end when the main thread waits for them); the main thread's look at the stopped session every
// millisecond instead of every 17 (it still leaves only once the session's work is done).
bool SessionJoinEndsSleep(std::uint32_t sleep_return);
std::int64_t SessionPollSleep(std::uint32_t return_address, std::int64_t ns);

}  // namespace Port::Mh3u
