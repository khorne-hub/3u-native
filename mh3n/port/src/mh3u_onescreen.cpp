// MH3U (mh3u.h): the single screen as MH4U's (onescreen.cpp): the touch screen's menus drawn on the top screen by the
// game itself, in place, while the touch screen is not shown; the touch screen's own picture never comes over the main
// one.
//
// MH3U's menus are interface units (uID*, each a uGUI): sUnit::draw (0x005a0910) draws a unit in viewport v when bit
// 16 + v of its flags (cUnit + 4) is set, and the top and the touch screens are viewports of their own (sCamera's,
// mh3u_game.cpp reads which). A unit of a touch-screen menu given the top screen's viewport bits instead is drawn there
// by the game, with its text, its animations and its cursor, at the top screen's resolution: the GUI keeps its pixels,
// so its 320 x 240 lands in the 400 x 240 from the left (seen in MH3U.exe, Character Select's plates). The uGUI draw hook
// (mh3u_gfx.inc, Mh3uGuiDrawOne) then scales and moves it where it goes (OneScreenDraw): its projection block's scale
// times `size` round the GUI's corner, then moved, as the HUD's parts are.
//
// Once a frame (Step, from mh3u_game.cpp after the touch menu is known) the units of the touch menu (the units
// GameState's touch_menu counts: drawn on the touch screen alone, not its panel at rest) get the top screen's bits and
// a place (Arrange: a block for the whole menu by GameState's touch_menu_place, and the rules below for some classes);
// what was written is noted, and given back as the game had it when the unit is no longer placed, the menu closes, the
// option goes off or the player shows the touch screen (SetTouchScreenShown: the app's corner, stacked, side by side, a
// second display or the town's large screen: the menus then stay on it, as on the console). A unit left out of the
// arrangement stays on the touch screen, which is not shown: it is not seen. The menus are worked with the buttons, as
// MH4U's single screen's are (the controls over them drawn faint, the app's touch.cpp menus_area).
//
// The game's other readers of a unit's viewport bits see the game's own: mh3u_game.cpp reads the units through GameFlags.
#include "mh3u_game.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

namespace Port::Mh3u {

namespace {
std::atomic<bool> touch_shown{false};
}  // namespace

void SetTouchScreenShown(bool shown) {
    touch_shown.store(shown, std::memory_order_relaxed);
}

namespace OneScreen {

namespace {

constexpr u32 UNIT_FLAGS = 0x04;
constexpr int VIEW_SHIFT = 16;
constexpr u32 VIEW_BITS = 0x3ffu << VIEW_SHIFT;

/// Where a unit of the touch screen is drawn on the top one: p (the touch screen's pixels, 320 x 240) goes to
/// scale * p + (tx, ty) in the top screen's (the console's 400 x 240, centred in a wider picture). `touch`: left on the
/// touch screen (not shown).
struct Spot {
    float scale = 1.0f, tx = 0.0f, ty = 0.0f;
    bool touch = false;
};
constexpr Spot Block(float scale, float tx, float ty) {
    return Spot{scale, tx, ty, false};
}
constexpr Spot Leave() {
    return Spot{1.0f, 0.0f, 0.0f, true};
}

// The whole menu's block by GameState's touch_menu_place (mh3u_game.cpp's MENU_PLACES and the rules there), where the
// app showed the touch screen's picture before: 0 large in the middle (the main screen behind it a scene or a
// window that waits on the menu), 1 at the main screen's right (the hunter in its middle), 2 at its left (a list at its
// right), 3 between and below the main screen's lists (a list the game draws there, the chosen item's details beside
// it), 4 above the main screen's talk box (its lowest 30 %: the quest counter's receptionist talks while the touch
// screen has the quests).
// Place 3's units are only the touch screen's OK and Back (and the house's or Port Tanzia's panel beside them): A and B
// do them, and the lists they go with are the main screen's own, which reach its right edge and bottom (the item box's
// and the Equipment Box's ten columns, the Chief's Son's Generate Resources prices, the START menu's Item Pouch). Small
// in the corner, as the 3DS single-screen mod of MH4U had them, they covered the last column's two lowest cells and a
// price's last figure (the Moto, mh3u-menus and mh3u-menus2); MH4U's own single screen draws no OK or Back over its
// lists (its Item Pouch, its Armory's lists and its Equip. Box of ten columns: the Moto, mh3u-menus2): so here they stay
// on the touch screen, not shown, and nothing covers the list.
// Place 4 is at the main screen's right, as MH4U's NPC menus are (its Armory: the list at the right, the shop's sign at
// the top left): the top screen's own title sign of an NPC's menu ("Quests", "Stockpile": x 0 to 175, y 0 to 75 of the
// 400 x 240) stays whole, the zenny and points boxes at the top right (y 0 to 20) above the menu's top (the Cook's frame
// at the touch screen's y 25, the counters' tabs at 29: from 21 here), and the talk box below it (from y 164: the
// card's bottom, the touch screen's 235, at 160). In the middle (0.69 at x 89.6) the counters' tabs, the Chief's Son's
// and the Cook's menus covered the end of the sign (the Moto, mh3u-menus2).
constexpr Spot PLACES[] = {
    Block(0.80f, 72.0f, 24.0f),     // 0: 256 x 192 in the middle
    Block(0.62f, 199.6f, 89.2f),    // 1: at the right, low
    Block(0.58f, 2.0f, 95.0f),      // 2: at the left, low (a facility's list: Kayamba's masks and dances, 0.62 touched the
                                    // details at the right with Ooga-Boo Dance's end; PC, mh3u-ultimos)
    Leave(),                        // 3: the lists are the main screen's: OK and Back left on the touch screen (above)
    Block(0.66f, 187.0f, 5.0f),     // 4: 211 x 158 at the top right (the touch screen's x 13 to 315 at 196 to 395)
};

// (Neko's ferry and its Friend Search: place 4's block 16 rows higher, Step)
constexpr Spot FERRY = Block(0.66f, 187.0f, -11.0f);

/// Some classes of a menu placed apart from its block, in the situation `when` says (a unit drawn on either screen, as
/// the game draws it; with `when_top`, drawn on the top screen by the game itself; "" any) and the part of the game (0
/// any, 1 the front end, 2 a town, 3 a quest). The first rule that fits a unit wins.
struct Rule {
    std::string_view when;
    int where;
    std::string_view unit;  // ("*": any unit of the menu)
    Spot spot;
    bool when_top = false;
};
constexpr Rule RULES[] = {
    // the main menu: New Game, Continue, Options and DLC (the touch screen's 2 x 2 at x 5 to 320, y 7 to 158) at the top
    // screen's bottom, under the title's logo, above its copyright line (MH3U.exe, run u1)
    {"", 1, "uIDStartMenuNewGame", Block(0.58f, 106.0f, 122.0f)},
    {"", 1, "uIDStartMenuContinue", Block(0.58f, 106.0f, 122.0f)},
    {"", 1, "uIDStartMenuOption", Block(0.58f, 106.0f, 122.0f)},
    {"", 1, "uIDStartMenuDownload", Block(0.58f, 106.0f, 122.0f)},
    // Character Select: while "Use this character?" asks (its window, uIDCommonBack, on the top screen) nothing of the
    // touch screen's: the window has the buttons, the hunter is behind it. Else the three saves' plates at the top
    // screen's right, beside the hunter in its middle, OK and Back under them (place 1's block); the help line is the
    // frame's (uIDStartMenuCommonFrame, the title's own), which stays on the touch screen
    // (the same window on the touch screen is a menu's own: the DLC's "Connect to the Internet...?" with its Yes and No,
    // uIDCommonBack and two uIDPanelButton, all on the touch screen, comes as the place's block; MH3U.exe, run t4)
    {"uIDCommonBack", 1, "*", Leave(), true},
    {"", 1, "uIDContinuePlate", Block(0.60f, 240.0f, 40.0f)},
    // the character's creation (New Game): its menus (Name, Gender, Clothing... and each one's list, the hairstyles' and
    // the faces' grids) at the top screen's left, small, so the hunter in its middle shows what is picked (as one block,
    // place 0, they covered the hunter's face and hair: the Moto, mh3u-menus), OK and Back under them
    {"", 1, "uIDMakeCharacter", Block(0.55f, 2.0f, 50.0f)},
    {"uIDMakeCharacter", 1, "uIDStartMenuCommonReturn", Block(0.55f, 2.0f, 50.0f)},
    {"uIDMakeCharacter", 1, "uIDStartMenuCommonEnter", Block(0.55f, 2.0f, 50.0f)},
    // (and the title's frame with its help line, which mh3u_game.cpp counts as the creation's: "Cancel character
    // creation?" over its Yes and No, what each choice does)
    {"uIDMakeCharacter", 1, "uIDStartMenuCommonFrame", Block(0.55f, 2.0f, 50.0f)},
    // the main menu's Options (Sound, Effect and Game Settings, Language) under the screen's "Game Options" title
    {"", 1, "uIDOptMenuSoundSet", Block(0.70f, 88.0f, 56.0f)},
    {"", 1, "uIDOptMenuPlayingSet", Block(0.70f, 88.0f, 56.0f)},
    {"", 1, "uIDOptMenuGameSet", Block(0.70f, 88.0f, 56.0f)},
    {"", 1, "uIDOptMenuLanguageSet", Block(0.70f, 88.0f, 56.0f)},
    // the START menu (its list on the top screen): the touch screen's Customize Panels, Panel Sets and Hunter Search
    // Position (uIDCommonCustomRoot: the touch panels', not shown in a single screen) stay, and so do OK and Back, as in
    // place 3 (A and B do them; MH4U's START menu and its Item Pouch show none: in the corner they covered the end of
    // the Item Pouch's last row, the Moto, mh3u-menus2). (The condition is the unit on the touch screen, where it stays:
    // with a condition on the top screen alone OK and Back went to the START's block, low in the middle of the picture,
    // over the hunter: the Moto, mh3u-menus)
    {"", 0, "uIDCommonCustomRoot", Leave()},
    {"uIDCommonCustomRoot", 0, "uIDPanelButton", Leave()},
    // the Canteen (the Itinerant Cook's Select Meal and Ingredients, uIDPanelScrollCanteen with its buttons): the top
    // screen has its own panel at the bottom right (Cost, Ing A, Ing B, Eff), which the NPCs' place at the right covered
    // with the list's Back (the Moto, mh3u-menus2): at the left instead, under the "Canteen" sign (y 0 to 75), the
    // touch screen's x 13 to 315 at 2 to 177, its y 25 to 235 at 78 to 200
    {"", 2, "uIDPanelScrollCanteen", Block(0.58f, -5.5f, 63.5f)},
    {"uIDPanelScrollCanteen", 2, "uIDPanelButton", Block(0.58f, -5.5f, 63.5f)},
    // the quest counter's: Port Tanzia's "Hunter Search" frame at the touch screen's top (uIDLobbyOnline: its bar with
    // Close), drawn there all the time and a menu's unit only while the town's panel is not, stays; the same unit is
    // Neko's ferry's own menu (Choose a Port: Solo, Multi, Friend Search) in the Moga village, which comes (the Moto, run
    // moto1: left out by a rule for every town, the ferry showed only its Back)
    {"uIDLobbyShoQueOrder0", 2, "uIDLobbyOnline", Leave()},
    {"uIDLobbyShoQueOrder6", 2, "uIDLobbyOnline", Leave()},
    {"uIDLobbyShoQueOrder7", 2, "uIDLobbyOnline", Leave()},
    {"uIDPanelScrollGuild", 2, "uIDLobbyOnline", Leave()},
    {"uIDLobbyStamp", 2, "uIDLobbyOnline", Leave()},
};

/// A unit this file gave the top screen's bits: its class (another unit at the same address starts a note of its own),
/// the game's own bits and what was written, and its spot this frame.
struct Note {
    u32 vtable = 0;
    u32 game_views = 0, written_views = 0;
    Spot spot;
    u32 frame = 0;  // (the frame it was placed last)
};
std::unordered_map<u32, Note> notes;  // unit address -> note

// The touch parts (Mh3u::OneScreenParts, the uGUI draw hook's Mh3uGuiDrawTouchPart): a unit the game draws on both
// screens whose nodes say their screens has some for the touch screen alone, which the single screen draws on the top
// screen too: uIDCommonFrame's help line (what a menu asks: "Take how many?", "Choose a Mask to wear.", "You can't change
// Dances...") and the item's panel of a box or a shop (the item, how many the pouch and the box hold and, while it asks,
// the quantity and the total: the RG406V and the Moto, mh3u-menus2, saw a quantity asked that nothing showed), and a
// facility's list title (Kayamba's "Mask List", uIDLobbyShoCha1). With a menu of one block they come in that block, as
// on the touch screen (its help line under its list); with the lists on the main screen (place 3) each where PARTS_3 says.
struct PartNote {
    u32 vtable = 0;
    std::vector<OneScreenPart> parts;
};
std::unordered_map<u32, PartNote> part_notes;  // unit address -> its parts this frame
// (both screens' units with nothing of theirs on the touch screen to bring: the font's)
constexpr std::string_view NO_PARTS[] = {"uIDFont"};
struct PartRule {
    std::string_view unit;
    int node;  // (its touch nodes' order, -1 all)
    Spot spot;
};
// Place 3 (a list of the game's on the main screen, its item's details at its left: the house's box, the Market, the
// supply box, the rewards): uIDCommonFrame's touch nodes are the help line (0: "Take how many?", "Select where to place
// the item.") and the item's panel (1: the item, the pouch's and the box's counts, and the quantity and the total while
// it asks how many). The item's panel over the main screen's sign at the top left (the touch screen's x 17 to 202, y 38
// to 170 with the Market's Total at 2 to 105, 2 to 76: the sign's x 0 to 175, y 0 to 75; the item's details below it,
// from y 77, stay whole);
// the help line along the main screen's bottom left, under the details (the touch screen's x 5 to 262, y 169 to 240 at
// 2 to 156, 195 to 238). PC, mh3u-ultimos (each node drawn as it is first: its place on the top screen is the touch
// screen's, from the left)
constexpr PartRule PARTS_3[] = {
    {"uIDCommonFrame", 0, Block(0.60f, -1.0f, 94.0f)},
    {"uIDCommonFrame", 1, Block(0.56f, -8.0f, -19.3f)},
};
// The quest's rewards (aResult, out of a town and a quest): the main screen's top left is the rewards' Time (x 1 to 98, y
// 0 to 24), which the item's panel covered (PC, mh3u-hub: "T[the item's icon]e: 11"); there its panel goes right of the
// Time, still above the Quest Rewards menu (from y 50) and left of the Reward Box (from x 224): its icon, name and counts
// at x 101 to 194, y 0 to 36
constexpr Spot RESULT_ITEM = Block(0.56f, 92.0f, -19.3f);
std::mutex notes_mutex;               // (Step and the draw hook run on the emulation thread; the lock is cheap)

// (research: mh3u_one, the runner's --set / MH4N_SET_LIVE) "place <n> <scale> <tx> <ty>", "unit <class> <scale> <tx>
// <ty>", "unit <class> touch", "taps [0|1]" (every menu drawn as one block takes a finger: to try the app's mapping
// anywhere), "clear"
struct Override {
    std::string unit;
    int place = -1;
    Spot spot;
    int part = -2;  // ("part <class> <node> ...": a touch part's spot, node -1 all of them; -2 not a part)
};
std::vector<Override> overrides;
std::mutex overrides_mutex;
std::atomic<bool> taps_anywhere{false};  // (research, mh3u_one = taps: every menu of one block takes a finger)

std::string arrangement_logged;

/// Whether a unit of that class is drawn: on the top screen (`top_views`) with `top`, else in any viewport (the units'
/// flags are the game's, GameFlags)
bool Drawn(const UnitView* units, std::size_t count, std::string_view name, u32 top_views, bool top) {
    const u32 views = top ? top_views : 0x3ffu;
    for (std::size_t i = 0; i < count; ++i) {
        if (name == units[i].name && (units[i].flags & 0x4800) == 0x4800 &&
            ((units[i].flags >> VIEW_SHIFT) & views) != 0) {
            return true;
        }
    }
    return false;
}

Spot SpotOf(std::string_view name, int place, int where, const UnitView* units, std::size_t count, u32 top_views) {
    {
        std::lock_guard lock(overrides_mutex);
        for (const Override& o : overrides)
            if (!o.unit.empty() && o.part == -2 && o.unit == name) return o.spot;
    }
    for (const Rule& r : RULES) {
        if (r.unit != "*" && r.unit != name) continue;
        if (r.where != 0 && r.where != where) continue;
        if (!r.when.empty() && !Drawn(units, count, r.when, top_views, r.when_top)) continue;
        return r.spot;
    }
    {
        std::lock_guard lock(overrides_mutex);
        for (const Override& o : overrides)
            if (o.unit.empty() && o.part == -2 && o.place == place) return o.spot;
    }
    // (a town's menu not known yet, large in the middle by its place: above the main screen's talk box instead, as the
    // quest counter's, for an NPC most often talks there while its menu is up)
    if (place == 0 && where == 2) place = 4;
    return PLACES[std::clamp(place, 0, static_cast<int>(std::size(PLACES)) - 1)];
}

/// Gives a unit's bits back as the game had them, if they are still what was written.
void GiveBack(const Guest& g, u32 unit, const Note& note) {
    const u32 flags = g.R32(unit + UNIT_FLAGS);
    if (g.R32(unit) == note.vtable && (flags & VIEW_BITS) == note.written_views) {
        g.W32(unit + UNIT_FLAGS, (flags & ~VIEW_BITS) | note.game_views);
    }
}

}  // namespace

void Reset() {
    std::lock_guard lock(notes_mutex);
    notes.clear();
    part_notes.clear();
    arrangement_logged.clear();
}

u32 GameFlags(u32 unit, u32 flags) {
    std::lock_guard lock(notes_mutex);
    const auto it = notes.find(unit);
    if (it == notes.end() || (flags & VIEW_BITS) != it->second.written_views) return flags;
    return (flags & ~VIEW_BITS) | it->second.game_views;
}

bool SetOption(const std::string& value) {
    char what[16] = {}, cls[96] = {};
    float s = 1, x = 0, y = 0;
    std::lock_guard lock(overrides_mutex);
    if (std::sscanf(value.c_str(), "%15s", what) != 1 || std::string_view(what) == "clear") {
        overrides.clear();
        Log("one screen: the research places cleared");
        return true;
    }
    if (std::string_view(what) == "taps") {
        int on = 1;
        std::sscanf(value.c_str(), "%*s %d", &on);
        taps_anywhere = on != 0;
    } else if (std::string_view(what) == "place") {
        int n = -1;
        if (std::sscanf(value.c_str(), "%*s %d %f %f %f", &n, &s, &x, &y) != 4 || n < 0) return false;
        overrides.erase(std::remove_if(overrides.begin(), overrides.end(),
                                       [n](const Override& o) { return o.unit.empty() && o.place == n; }),
                        overrides.end());
        overrides.push_back({"", n, Block(s, x, y)});
    } else if (std::string_view(what) == "part") {
        int node = -1;
        char rest[16] = {};
        if (std::sscanf(value.c_str(), "%*s %95s %d %15s", cls, &node, rest) < 3) return false;
        Spot spot = Leave();
        if (std::string_view(rest) != "off") {
            if (std::sscanf(value.c_str(), "%*s %*s %*d %f %f %f", &s, &x, &y) != 3) return false;
            spot = Block(s, x, y);
        }
        const std::string name = cls;
        overrides.erase(std::remove_if(overrides.begin(), overrides.end(),
                                       [&name, node](const Override& o) { return o.unit == name && o.part == node; }),
                        overrides.end());
        overrides.push_back({name, -1, spot, node});
    } else if (std::string_view(what) == "unit") {
        char rest[16] = {};
        const int got = std::sscanf(value.c_str(), "%*s %95s %15s", cls, rest);
        if (got < 2) return false;
        Spot spot = Leave();
        if (std::string_view(rest) != "touch") {
            if (std::sscanf(value.c_str(), "%*s %*s %f %f %f", &s, &x, &y) != 3) return false;
            spot = Block(s, x, y);
        }
        const std::string name = cls;
        overrides.erase(std::remove_if(overrides.begin(), overrides.end(),
                                       [&name](const Override& o) { return o.unit == name; }),
                        overrides.end());
        overrides.push_back({name, -1, spot});
    } else {
        return false;
    }
    Log(fmt::format("one screen: research place {}", value));
    return true;
}

void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, u32 top_views,
          const std::vector<std::size_t>& menu_units, int place, bool tavern, Hooks::GameState& s) {
    const int where = s.front_end == 1 ? 1
                      : s.place == Hooks::GameState::PLACE_TOWN ? 2
                      : s.place == Hooks::GameState::PLACE_QUEST ? 3 : 0;
    // A page of the quest's touch panels: the Item Pouch's grid the quick items open ("Touch the item you want to use").
    // Its grid is the panels' own units (at rest for mh3u_game.cpp's count) and its only menu unit is its Back: drawn on
    // the main screen, the Back came alone and the grid stayed on the hidden touch screen (the Moto, mh3u-menus). Such a
    // page is left to the app's older way, the touch screen's picture large over the main one and taken by the finger
    // (game_screen_view), as the quick items opened it before (mh3u-final, mh3u-pantalla)
    bool panel_page = where == 3 && place == 0 && s.menu != 1 && !menu_units.empty();
    for (const std::size_t i : menu_units)
        if (std::string_view(units[i].name) != "uIDPanelButton") panel_page = false;
    // (The Market's and the house box's item panel with the quantity, which mh3u-menus2 had the touch screen shown
    // beside the main one for, is uIDCommonFrame's touch part: drawn on the top screen, PARTS_3.)
    const bool on = settings.single_screen && !touch_shown.load(std::memory_order_relaxed) && top_views != 0 &&
                    s.touch_menu == 1 && !panel_page;
    std::lock_guard lock(notes_mutex);
    std::string line;
    int placed = 0;
    bool one_block = true;  // (every unit placed with one spot: the block the app maps a finger through)
    Spot block{};
    // Neko's ferry and its Friend Search (uIDLobbyOnline a menu's unit in a town, no place of its own): the Friend
    // Search's "Player Select", the ports found (MULTI Port Tanzia, the host's name and HR) and their header are the
    // town panel's page (uIDPanelHunter, at rest otherwise), which stayed on the hidden touch screen: the ports were
    // chosen blind (PC, mh3u-ultimos, a hub with the Moto). The panel comes with the menu, all of it a little higher
    // (FERRY: the panel's own bar, Hunter Search and Close, at the touch screen's top, goes above the main screen's top)
    // In the Tavern (a hub's hall: no ferry there) the frame is the town panel's own, a menu's unit only while the panel
    // is down: the Quest Board's list and card ("Join this Quest?", Low, High, G) and the departure's fade after Confirm
    // Departure's Yes. Taken for the ferry, the Quest Board's card went 16 rows higher, its top above the main screen's,
    // and the Players panel showed for the departure's 2.5 s over the hunter (PC, mh3u-hub): there the frame stays on the
    // touch screen and the rest goes where the town's menus go (place 4).
    std::vector<std::size_t> placed_units = menu_units;
    bool ferry = false;
    bool board = false;  // (the Tavern's Quest Board: its help line asks "Join this Quest?" over Yes and No)
    for (const std::size_t i : menu_units) {
        if (where == 2 && place == 0 && !tavern && std::string_view(units[i].name) == "uIDLobbyOnline") ferry = true;
        if (std::string_view(units[i].name) == "uIDLobbyQuestBoardRoot") board = true;
    }
    if (ferry) {
        for (std::size_t i = 0; i < count; ++i) {
            const u32 views = (units[i].flags >> VIEW_SHIFT) & 0x3ff;
            if (std::string_view(units[i].name) == "uIDPanelHunter" && (units[i].flags & 0x4800) == 0x4800 &&
                views != 0 && (views & top_views) == 0) {
                placed_units.push_back(i);
            }
        }
    }
    if (on) {
        for (const std::size_t i : placed_units) {
            const UnitView& unit = units[i];
            const Spot spot = ferry ? FERRY
                              : tavern && std::string_view(unit.name) == "uIDLobbyOnline"
                                  ? Leave()
                                  : SpotOf(unit.name, place, where, units, count, top_views);
            line += fmt::format(" {}", unit.name);
            if (spot.touch) {
                line += " (left on the touch screen)";
                continue;
            }
            line += fmt::format(" {:.2f} {:.0f},{:.0f}", spot.scale, spot.tx, spot.ty);
            const u32 vtable = g.R32(unit.address);
            const u32 flags = g.R32(unit.address + UNIT_FLAGS);
            const u32 want = top_views << VIEW_SHIFT;
            auto it = notes.find(unit.address);
            if (it != notes.end() && it->second.vtable != vtable) {
                notes.erase(it);
                it = notes.end();
            }
            if (it == notes.end()) {
                it = notes.emplace(unit.address, Note{vtable, flags & VIEW_BITS, want, spot, frame}).first;
            } else if ((flags & VIEW_BITS) != it->second.written_views) {
                it->second.game_views = flags & VIEW_BITS;  // (the game set them again)
            }
            it->second.written_views = want, it->second.spot = spot, it->second.frame = frame;
            if ((flags & VIEW_BITS) != want) g.W32(unit.address + UNIT_FLAGS, (flags & ~VIEW_BITS) | want);
            if (placed == 0) block = spot;
            else if (spot.scale != block.scale || spot.tx != block.tx || spot.ty != block.ty) one_block = false;
            ++placed;
        }
    }
    // the units no longer placed: back as the game had them
    for (auto it = notes.begin(); it != notes.end();) {
        if (it->second.frame == frame && on) {
            ++it;
            continue;
        }
        GiveBack(g, it->first, it->second);
        it = notes.erase(it);
    }
    // the touch parts of the units drawn on both screens
    part_notes.clear();
    std::string parts_line;
    for (std::size_t i = 0; on && i < count; ++i) {
        const UnitView& unit = units[i];
        const u32 views = (unit.flags >> VIEW_SHIFT) & 0x3ff;
        if ((unit.flags & 0x4800) != 0x4800 || (views & top_views) == 0 || (views & ~top_views) == 0) continue;
        if (std::find(std::begin(NO_PARTS), std::end(NO_PARTS), std::string_view(unit.name)) != std::end(NO_PARTS)) {
            continue;
        }
        std::vector<OneScreenPart> parts;
        bool research = false;
        {
            std::lock_guard olock(overrides_mutex);
            for (const Override& o : overrides) {
                if (o.part == -2 || o.unit != unit.name) continue;
                research = true;
                if (!o.spot.touch) parts.push_back({o.part, o.spot.scale, o.spot.tx, o.spot.ty});
            }
        }
        if (!research) {
            if (placed > 0 && one_block && std::string_view(unit.name) == "uIDCommonFrame") {
                // (its help line, node 0, in the block too, as on the touch screen under the menu; at the main screen's
                // right, an NPC's menu (place 4), it covered the main screen's own panel below the menu, Kayamba's
                // Status, and the NPC's talk: there it stays off, as MH4U's NPC menus show no help line. The Quest
                // Board is no NPC: its line is the question, "Join this Quest?", and then "You have joined the Quest.
                // Stand by at the Quest Departure Gate to the left.", without which Yes and No asked nothing)
                parts.push_back({1, block.scale, block.tx, block.ty});
                if (ferry || board || (place != 4 && !(place == 0 && where == 2))) {
                    parts.push_back({0, block.scale, block.tx, block.ty});  // (the ferry's: "Choose a destination.")
                }
            } else if (placed > 0 && one_block) {
                parts.push_back({-1, block.scale, block.tx, block.ty});
            } else if (place == 3) {
                for (const PartRule& r : PARTS_3) {
                    if (r.unit != unit.name || r.spot.touch) continue;
                    const Spot spot = where == 0 && r.node == 1 ? RESULT_ITEM : r.spot;
                    parts.push_back({r.node, spot.scale, spot.tx, spot.ty});
                }
            }
        }
        if (parts.empty()) continue;
        for (const OneScreenPart& part : parts)
            parts_line += fmt::format(" {}#{} {:.2f} {:.0f},{:.0f}", unit.name, part.node, part.size, part.dx, part.dy);
        part_notes[unit.address] = PartNote{g.R32(unit.address), std::move(parts)};
    }
    if (!parts_line.empty()) line += "; touch parts" + parts_line;
    // (on, the touch screen's picture never comes over the main one: a menu whose units are all left on the touch
    // screen, as Character Select's while "Use this character?" asks, is not seen at all)
    s.touch_menu_top = on ? 1 : 0;
    if (placed > 0 && one_block) {
        s.touch_menu_block[0] = block.scale, s.touch_menu_block[1] = block.tx, s.touch_menu_block[2] = block.ty;
        // (a quest's menus of the touch panels' own, large in the middle: the Item Pouch's grid, the Combo List; they
        // hold no window of the game's, so only a finger works them)
        s.touch_menu_taps = (where == 3 && place == 0 && s.menu != 1) || taps_anywhere ? 1 : 0;
    }
    if (settings.trace_state) {
        const std::string now = on ? fmt::format("place {}:{}", place, line) : std::string("off");
        if (now != arrangement_logged) {
            arrangement_logged = now;
            Log(fmt::format("one screen: frame {} {}", frame,
                            on ? fmt::format("the touch menu on the top screen, {}", now) : std::string("nothing placed")));
        }
    }
}

int Parts(u32 unit, OneScreenPart* parts, int max) {
    std::lock_guard lock(notes_mutex);
    const auto it = part_notes.find(unit);
    if (it == part_notes.end()) return 0;
    int n = 0;
    for (const OneScreenPart& part : it->second.parts) {
        if (n == max) break;
        parts[n++] = part;
    }
    return n;
}

bool Draw(u32 unit, float* size, float* dx, float* dy) {
    std::lock_guard lock(notes_mutex);
    const auto it = notes.find(unit);
    if (it == notes.end()) return false;
    *size = it->second.spot.scale, *dx = it->second.spot.tx, *dy = it->second.spot.ty;
    return true;
}

}  // namespace OneScreen

bool OneScreenDraw(std::uint32_t unit, float* size, float* dx, float* dy) {
    if (!Active()) return false;
    return OneScreen::Draw(unit, size, dx, dy);
}

int OneScreenParts(std::uint32_t unit, OneScreenPart* parts, int max) {
    if (!Active()) return 0;
    return OneScreen::Parts(unit, parts, max);
}

}  // namespace Port::Mh3u
