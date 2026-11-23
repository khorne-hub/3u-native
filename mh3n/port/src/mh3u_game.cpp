// MH3U (mh3u.h): the game's structures as the port reads them, once a frame. Addresses are USA v1.0's (the only version
// of the game); docs/MH3U.md lists them with how each was found.
#include "mh3u_game.h"

#include "mh3u_classes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

namespace Port::Mh3u::Game {

namespace {

// ── The engine's systems (each class's constructor stores its instance there)
constexpr u32 SAREA = 0x00ca8910;        // sArea*: the areas alive
constexpr u32 AREA_COUNT = 0x3818, AREA_TABLE = 0x3820;  // (as MH4U's: 8 cArea*, the last one on top)
constexpr u32 SUNIT = 0x00ca898c;        // sUnit*: every unit, by line (sUnit::draw 0x005a0910)
constexpr u32 SUNIT_LINES = 0x614;       // u32: how many lines
constexpr u32 LINE_FIRST = 0x20, LINE_SIZE = 0x18;  // a line's first unit (sUnit + 0x20 + 0x18 * line)
constexpr u32 UNIT_FLAGS = 0x04;         // bits 0-2 the unit's state (2 alive), bits 16-25 the viewports it is drawn in
constexpr u32 UNIT_NEXT = 0x0c;
constexpr u32 UNIT_ALIVE = 2;
constexpr int VIEW_SHIFT = 16;
constexpr u32 UNIT_DRAWN = 0x4800;      // cUnit + 4: the two draw bits sUnit::draw wants (an interface unit alive but
                                         // idle, as the START menu's pouch while it is closed, has them clear)
// The game's interface (sID, the ID system: its menus, talks, HUD and touch panels)
constexpr u32 SID = 0x00c406a0;          // sID*
constexpr u32 SID_WINDOW = 0x18;
// The local hunter (mh3u-qol's finds): sPlayer [0x00c40738] + 0x20 + 4 * n, n this console's player number (sGame
// [0x00c3b978] + 0x2b, s8); its state block [hunter + 0xd5c]: + 0x02 the weapon's kind (MH4U's numbers: 10 the bow),
// + 0x3d 3 under water
constexpr u32 SPLAYER = 0x00c40738, SGAME = 0x00c3b978;
constexpr u32 SPLAYER_UNITS = 0x20, SGAME_PLAYER = 0x2b, HUNTER_STATE = 0xd5c;
constexpr u32 STATE_WEAPON = 0x02, STATE_WATER = 0x3d;
constexpr u32 STATE_DRAWN = 0x58;          // u8: 1 the weapon out, 0 sheathed (mh3u-pantalla's per-frame trace)
constexpr u8 WATER_UNDER = 3;         // u32: 10 while a window has the buttons (the START menu, a talk), 9..0 as it
                                         // closes (a fade of 10 frames), 0 the hunter free (mh3u-pantalla's trace)

const char* ClassName(u32 vtable) {
    const auto* first = std::begin(kClassVtables);
    const auto* last = std::end(kClassVtables);
    const auto* it = std::lower_bound(first, last, vtable, [](const ClassVtable& c, u32 v) { return c.vtable < v; });
    return it != last && it->vtable == vtable ? it->name : nullptr;
}

bool Starts(const char* name, const char* prefix) {
    return name != nullptr && std::string_view(name).substr(0, std::string_view(prefix).size()) == prefix;
}

using Unit = UnitView;

// sCamera's viewports (mh3u-gfx's find): 8 at + 0x28, 0x198 bytes each, + 0x10 visible, + 0x15 the screen it shows on
constexpr u32 SCAMERA = 0x00ca8914;
constexpr u32 VIEWPORTS = 0x28, VIEWPORT_SIZE = 0x198, VIEWPORT_COUNT = 8, VIEWPORT_VISIBLE = 0x10,
              VIEWPORT_DISPLAY = 0x15;

// The touch screen's interface units that are its panel at rest (the town's map and tabs, the quest's panels): the single
// screen leaves the touch screen hidden while only these are on it. Any other one on the touch screen is a menu or a
// window the player works there, and the single screen has the game draw it on the main one (mh3u_onescreen.cpp).
constexpr std::string_view REST_UNITS[] = {
    // the towns' panel (Hunter Search: the map and its tabs, Hunters for Hire with Guild Cards, Items and Gestures)
    "uIDLobbyTop", "uIDPanelHunter", "uIDCommonBG",
    // the title's touch screen (its background, PRESS A and the frame round the menus: the menus are their own units,
    // uIDStartMenuNewGame, Continue, Option, Download, uIDContinuePlate...)
    "uIDStartMenuCommonBG", "uIDStartMenuPress", "uIDStartMenuCommonFrame",
    // the quests' panels (the player's own set: uIDPanelModule holds them; uIDPanelButton is drawn only while one of
    // them opens its list on the touch screen, the pouch's "Touch the item you want to use", and is a menu then)
    "uIDPanel", "uIDPanelCustom", "uIDPanelModule", "uIDPanelButtonCamera",
    "uIDPanelButtonMultiMogaInfoTab", "uIDPanelButtonMultiCrosskey", "uIDPanelButtonWeapon", "uIDPanelButtonDetail",
};
// How a menu on the touch screen is best shown (GameState's touch_menu_place): by its class, the first that matches;
// else large in the middle. 1: at the right of the main screen (the main screen shows the hunter or a list that goes
// with it); 4: large above the main screen's bottom band, where the game's talk box is
struct MenuPlace {
    std::string_view unit;
    int place;
};
constexpr MenuPlace MENU_PLACES[] = {
    {"uIDContinuePlate", 1},     // the save's choice: the hunter in the middle of the main screen
    {"uIDCommonCustomRoot", 2},  // the START menu's panel options: its list is at the main screen's right
    // the quest counter's (Port Tanzia's receptionist, mh3u-verifica's run a): the quests' levels (uIDLobbyShoQueOrder0:
    // Lv 1, Lv 2, Event Quests), their list and the chosen quest's card (uIDPanelScrollGuild, with uIDPanelButtonGuild
    // for the list's rows) and the card's stamp (uIDLobbyStamp), while the receptionist talks in the main screen's talk
    // box ("Bon voyage!", its name above it: the main screen's lowest 30 %): both seen, the card above the talk
    {"uIDLobbyShoQueOrder0", 4}, {"uIDLobbyShoQueOrder6", 4}, {"uIDLobbyShoQueOrder7", 4},
    {"uIDPanelScrollGuild", 4}, {"uIDPanelButtonGuild", 4}, {"uIDLobbyStamp", 4},
};
// The touch screen's units that are its panel at rest while the town's panel (uIDLobbyTop: its map and tabs) is drawn
// beside them: Port Tanzia's "Hunter Search" frame (uIDLobbyOnline, drawn there all the time; in the Moga village only
// with the ports' menu, which takes the panel's place) and the quest counter's stamp (uIDLobbyStamp, alive through the
// receptionist's whole talk). Without the panel they are a menu's. So in Port Tanzia an NPC's talk leaves the touch
// screen hidden, as in the village, and its talk box is seen (before, the town's panel came large over it)
constexpr std::string_view REST_WITH_TOWN_PANEL[] = {"uIDLobbyOnline", "uIDLobbyStamp"};
bool RestWithTownPanel(std::string_view name) {
    return std::find(std::begin(REST_WITH_TOWN_PANEL), std::end(REST_WITH_TOWN_PANEL), name) !=
           std::end(REST_WITH_TOWN_PANEL);
}
// The touch screen's units that only go with a list the game draws on the MAIN screen: the panel with the chosen item,
// the help line and OK / Back (uIDPanelButton), and the house's (uIDLobbyMyhRoot). The item box, the supply box, a
// shop, the Outfitter's armory and the quest's rewards have their list and its item's details on the main screen, at
// its left and right (mh3u-faltan's shots i_h7, c_box3, i_o7, c_r1; a shop, mh3u-final's): while a window of the game's
// has the buttons (sID's count) and these are the only menu units on the touch screen, the touch screen goes below
// the main screen's lists, both screens whole (place 3), not the touch screen large over them. The supply box and the
// rewards hold no window (sID's count stays 0 there, the Moto's run): their list's own unit on the main screen says
// it (TOP_LISTS). (The quest's Item Pouch grid, also a uIDPanelButton with the panels' own unit, holds no window and
// draws no list on the main screen: large in the middle, as before.)
constexpr std::string_view WITH_LIST_ON_TOP[] = {"uIDPanelButton", "uIDLobbyMyhRoot"};
bool WithListOnTop(std::string_view name) {
    return std::find(std::begin(WITH_LIST_ON_TOP), std::end(WITH_LIST_ON_TOP), name) != std::end(WITH_LIST_ON_TOP);
}
// a list of the game's drawn on the main screen alone: the supply box's (uIDCockpitBox; the shared one in a hall,
// uIDCockpitShareBox), the rewards' (uIDRewardBox, with uIDResultCommon's menu)
// The Smithy's and the Armory Clerk's lists in Port Tanzia (uIDLobbyEShop, its root) and a shop's (uIDLobbyShop) too: there
// the town's panel (uIDLobbyOnline, the Hunter Search) stays drawn on the touch screen beside the shop's buttons, and is
// not a menu of its own (mh3u-pc3's run g6: the Armory Clerk's list covered by the touch screen).
// The Moga village's Market (the Gal with the Goods: Buy, Sell, its item list, uIDLobbyMaki on the main screen; hiding
// that class hid the list, the Moto, mh3u-menus2) too.
constexpr std::string_view TOP_LISTS[] = {"uIDCockpitBox", "uIDCockpitShareBox", "uIDRewardBox", "uIDResultCommon",
                                          "uIDLobbyEShop", "uIDLobbyEShopRoot", "uIDLobbyShop", "uIDLobbyMaki"};
bool TopList(std::string_view name) {
    return std::find(std::begin(TOP_LISTS), std::end(TOP_LISTS), name) != std::end(TOP_LISTS);
}
// A window of the game's on the main screen that has the buttons while sID counts none: the common window
// (uIDCommonBack), as "Save your progress?" back from a quest (Port Tanzia's Tavern, the Moto, mh3u-menus: Yes and No
// with the hunt's controls, no menu pad). (On the touch screen the same unit is a menu's own, the DLC's question.)
constexpr std::string_view WINDOWS_ON_TOP[] = {"uIDCommonBack"};
bool WindowOnTop(std::string_view name) {
    return std::find(std::begin(WINDOWS_ON_TOP), std::end(WINDOWS_ON_TOP), name) != std::end(WINDOWS_ON_TOP);
}

// The town panel (uIDLobbyTop, drawn on the touch screen at rest): which page and row it shows. Its top row: the u8 at
// + 0x695, 1 while the hunter search's row is up (SOLO, Guild Cards, Items, Gestures: its Port Village button toggles
// it), 0 with the town's tabs (three dumps of the unit, the row toggled between them: the Moga village, woods1). The page
// (the map or Hunters for Hire) is not read yet: the map (2).
constexpr u32 LOBBY_TOP_ROW = 0x695;
u32 lobby_top = 0;  // (the frame's uIDLobbyTop drawn on the touch screen, from the units)
int TownPanel(const Guest& g) {
    const bool row = Guest::Heap(lobby_top) && g.R8(lobby_top + LOBBY_TOP_ROW) != 0;
    return 2 + (row ? 100 : 0);
}

// The touch screen's menus that are menus whatever sID's window count says: the item pouch and the combine list opened
// from the quest's touch panels (the hunter waits on them, crouched, while sID counts no window: mh3u-pantalla's run)
constexpr std::string_view ALWAYS_MENUS[] = {"uIDCommonPauch", "uIDCommonMixlist", "uIDCommonMix", "uIDPanelButton"};

bool Always(std::string_view name) {
    return std::find(std::begin(ALWAYS_MENUS), std::end(ALWAYS_MENUS), name) != std::end(ALWAYS_MENUS);
}

bool Rest(std::string_view name) {
    return std::find(std::begin(REST_UNITS), std::end(REST_UNITS), name) != std::end(REST_UNITS);
}
// A town's facility drawn on the main screen (the Shakalakas' Cha-Cha and Kayamba, uIDLobbyShoCha*: Change Mask, Select
// Dances, Select Abilities; the farm's, the ships' and the traders'): its list on the touch screen is made of
// uIDPanelButton rows (the masks, the dances), which place 3 took for OK and Back alone and left on the hidden touch
// screen (PC, mh3u-ultimos: Kayamba's Change Mask showed the mask's details and no list to pick from). Its menu goes
// at the main screen's left instead (place 2), the details at its right stay whole.
constexpr std::string_view FACILITIES[] = {"uIDLobbyShoCha", "uIDLobbyChacha", "uIDLobbyFarm", "uIDLobbyShoShip",
                                           "uIDLobbyShoTrade", "uIDLobbyShoPaper"};
bool Facility(const char* name) {
    for (const std::string_view f : FACILITIES)
        if (Starts(name, std::string(f).c_str())) return true;
    return false;
}
// The Moga Forecast (the Chief's Son's: uIDPanelButtonMultiMogaInfoTab, a page of the touch panels, Today, Tomorrow and
// the Day After with their monsters and resources) is a menu's in a town while a window of the game's has the buttons:
// at rest it stayed on the hidden touch screen and the forecast was not seen at all (PC, mh3u-ultimos)
constexpr std::string_view TOWN_MENU_PAGES[] = {"uIDPanelButtonMultiMogaInfoTab"};
bool TownMenuPage(std::string_view name) {
    return std::find(std::begin(TOWN_MENU_PAGES), std::end(TOWN_MENU_PAGES), name) != std::end(TOWN_MENU_PAGES);
}

// A town's item panel (uIDPanel on the touch screen, the quests' panels' class and at rest there): the chosen item, how
// many the pouch and the box hold, and while the Market's Buy or Sell or the house's box (Store, Remove: "Take how
// many?") asks a quantity, the quantity and the total. With the single screen it stayed on the hidden touch screen: a
// quantity was asked that nothing showed (the Moto and the RG406V, mh3u-menus2; MH4U's shops and box show the count
// on the main screen). Its contents are not drawn by its own unit, nor by the panel's other units (each given the top
// screen's viewport bits, the RG406V: nothing of it showed), so while such a list is on the main screen (uIDLobbyMaki)
// the item panel is a menu unit that goes with the list (place 3) and the single screen shows the touch screen whole
// beside the main one, as on the console (mh3u_onescreen.cpp). Not with a Smithy's or an Armory's list (uIDLobbyEShop
// on the main screen): they ask no quantity and stay on the single screen.
bool MarketPanel(std::string_view name, bool market) {
    return market && name == "uIDPanel";
}

std::vector<Unit> units;       // this frame's interface units alive (uID*, uAppGUI...)
// (research: mh3u_probe = <class> <what> <value...>, the runner's --set) what is done to that class's units each frame:
// "scale s", "move dx dy" (uCoord's), "hide" (no viewport), "view <mask>"
std::string probe_class, probe_what;
float probe_a = 0, probe_b = 0;
u32 probe_root = 0;
float probe_matrix[16] = {};
std::string units_signature;   // their classes and views, for trace_state 2
std::string area_name;         // the area on top
u32 views_top = 0, views_touch = 0;  // the viewports of each screen (bits), as sCamera has them
std::string touch_menu_unit;   // the unit that made the touch screen a menu's, for the log
u32 hunter_unit = 0, hunter_state = 0;  // the local hunter's unit and state block in a quest, for the log

}  // namespace

bool SetProbe(const std::string& value) {
    char cls[96] = {}, what[16] = {};
    float a = 0, b2 = 0;
    const int n = std::sscanf(value.c_str(), "%95s %15s %f %f", cls, what, &a, &b2);
    if (n < 2) {
        probe_class.clear();
        return true;
    }
    probe_class = cls, probe_what = what, probe_a = a, probe_b = b2;
    Log(fmt::format("probe: {} {} {} {}", probe_class, probe_what, probe_a, probe_b));
    return true;
}

void Reset() {
    Hud::Reset();
    Panels::Reset();  // (mh3u-pantalla: the quick items' panel)
    OneScreen::Reset();
    units.clear();
    units_signature.clear();
    area_name.clear();
}

std::string Describe() {
    return fmt::format("area {}, viewports top {:x} touch {:x}{}{}, hunter {:08x} state {:08x}", area_name, views_top,
                       views_touch, touch_menu_unit.empty() ? "" : ", touch menu ", touch_menu_unit, hunter_unit,
                       hunter_state);
}

Hooks::GameState Step(const Guest& g, u32 frame, const Settings& settings) {
    Hooks::GameState s;
    s.frame = frame;
    s.item_scroll = 0;  // (no hook turns MH3U's item bar: the app's L with Y or A does)
    // the area on top
    const u32 areas = g.R32(SAREA);
    const u32 count = Guest::Heap(areas) ? g.R32(areas + AREA_COUNT) : 0;
    const u32 top = count >= 1 && count <= 8 ? g.R32(areas + AREA_TABLE + 4 * (count - 1)) : 0;
    const char* area = Guest::Heap(top) ? ClassName(g.R32(top)) : nullptr;
    area_name = area ? area : (top ? fmt::format("{:08X}", g.R32(top)) : "-");
    const std::string_view a = area ? area : "";
    static int last_place = Hooks::GameState::PLACE_UNKNOWN, last_front = 0;
    if (a == "aVillage" || a == "aVillageSquare" || a == "aVillageFarm" || a == "aVillageHouse" || a == "aHarbor" ||
        a == "aHarborSquare" || a == "aHarborGuild" || a == "aLobby") {
        s.place = Hooks::GameState::PLACE_TOWN;  // (Moga Village and its parts, Port Tanzia and its Guild hall)
    } else if (a == "aQuest" || a == "aQuestArea" || a == "aFreeQuest" || a == "aFreeQuestArea") {
        s.place = Hooks::GameState::PLACE_QUEST;  // (a quest, or the Moga Woods' free hunt)
    } else if (a == "aStartMenu" || a == "aMenu" || a == "aOption" || a == "aDLContents" || a == "aGalleryMenu" ||
               a == "aGalleryDemo" || a == "aStaffroll" || a == "aViewer") {
        s.place = Hooks::GameState::PLACE_OTHER;  // (the title, the main menu, the save's choice, the options...)
        s.front_end = 1;
    } else if (a == "aResult" || a == "aArenaMenu") {
        s.place = Hooks::GameState::PLACE_OTHER;
    } else {
        s.place = last_place, s.front_end = last_front;  // (loading: aGame, aBoot; between areas: as it was)
    }
    last_place = s.place, last_front = s.front_end;
    // a window of the game's has the buttons: the START menu, a talk, a shop's or the box's list (sID's window count);
    // the front end is all menus (the title, the main menu, the save's choice)
    {
        const u32 id = g.R32(SID);
        const u32 window = Guest::Heap(id) ? g.R32(id + SID_WINDOW) : 0;
        s.menu = s.front_end == 1 || window != 0 ? 1 : 0;
        if (!Guest::Heap(id)) s.menu = s.front_end == 1 ? 1 : -1;
    }

    // the interface units alive, with the viewports they are drawn in
    units.clear();
    const u32 unit_system = g.R32(SUNIT);
    const u32 lines = Guest::Heap(unit_system) ? std::min<u32>(g.R32(unit_system + SUNIT_LINES), 64) : 0;
    for (u32 line = 0; line < lines; ++line) {
        u32 u = g.R32(unit_system + LINE_FIRST + LINE_SIZE * line);
        for (int n = 0; Guest::Heap(u) && n < 4096; ++n, u = g.R32(u + UNIT_NEXT)) {
            const char* name = ClassName(g.R32(u));
            if (name == nullptr || !(Starts(name, "uID") || Starts(name, "uAppGUI") || Starts(name, "uDrawID"))) continue;
            // (the viewport bits as the game set them: the single screen draws a touch menu's units on the top screen)
            const u32 flags = OneScreen::GameFlags(u, g.R32(u + UNIT_FLAGS));
            if ((flags & 7) != UNIT_ALIVE) continue;
            units.push_back({u, name, flags});
            if (!probe_class.empty() && probe_class == name) {
                if (probe_what == "scale") {
                    g.WF32(u + 0x60, probe_a), g.WF32(u + 0x64, probe_a);
                } else if (probe_what == "move") {
                    g.WF32(u + 0x40, probe_a), g.WF32(u + 0x44, probe_b);
                } else if (probe_what == "hide") {
                    g.W32(u + UNIT_FLAGS, flags & ~(0x3ffu << VIEW_SHIFT));
                } else if (probe_what == "view") {
                    g.W32(u + UNIT_FLAGS, (flags & ~(0x3ffu << VIEW_SHIFT)) | (static_cast<u32>(probe_a) << VIEW_SHIFT));
                } else if (probe_what == "iscale" || probe_what == "imove") {
                    // the GUI instance's root (uGUI + 0xf4, a cGUIInstRoot): its 4x4 matrix at + 0x10 (the screen's
                    // rotation; row 3 the translation), from the game's as first seen
                    const u32 root = g.R32(u + 0xf4);
                    if (Guest::Heap(root)) {
                        if (probe_root != root) {
                            probe_root = root;
                            for (int i = 0; i < 16; ++i) probe_matrix[i] = g.F32(root + 0x10 + 4 * i);
                        }
                        float m[16];
                        std::copy(std::begin(probe_matrix), std::end(probe_matrix), m);
                        if (probe_what == "iscale") {
                            for (int i = 0; i < 8; ++i) m[i] *= probe_a;
                        } else {
                            m[12] += probe_a, m[13] += probe_b;
                        }
                        for (int i = 0; i < 16; ++i) g.WF32(root + 0x10 + 4 * i, m[i]);
                    }
                }
            }
        }
    }
    Hud::Step(g, frame, settings, units.data(), units.size(), s.place == Hooks::GameState::PLACE_QUEST, s);

    // the local hunter in a quest: its weapon's kind and whether it swims
    if (s.place == Hooks::GameState::PLACE_QUEST) {
        const u32 players = g.R32(SPLAYER), game = g.R32(SGAME);
        const int n = Guest::Heap(game) ? static_cast<signed char>(g.R8(game + SGAME_PLAYER)) : -1;
        const u32 hunter = Guest::Heap(players) && n >= 0 && n < 8 ? g.R32(players + SPLAYER_UNITS + 4 * n) : 0;
        const u32 state = Guest::Heap(hunter) ? g.R32(hunter + HUNTER_STATE) : 0;
        hunter_unit = hunter, hunter_state = state;
        if (Guest::Heap(state)) {
            s.weapon = g.R8(state + STATE_WEAPON);
            s.underwater = g.R8(state + STATE_WATER) == WATER_UNDER ? 1 : 0;
            s.weapon_drawn = g.R8(state + STATE_DRAWN) != 0 ? 1 : 0;
            if (s.weapon == 10) s.ranged = 1;       // (MH4U's numbers: 10 the bow, 4 and 6 the bowguns)
            else if (s.weapon == 4 || s.weapon == 6) s.ranged = 2;
        }
    }
    // the weapon's own HUD in a quest says a bow or a bowgun (its arrows' coatings, its ammo): the touch controls' layout
    // for them
    if (s.place == Hooks::GameState::PLACE_QUEST) {
        for (const Unit& unit : units) {
            const std::string_view n = unit.name;
            if (n == "uIDCockpitBow") s.ranged = 1, s.weapon = 10;
            else if (n == "uIDCockpitGunner" && s.ranged == 0) s.ranged = 2, s.weapon = 4;
        }
    }
    Hud::SetBow(s.place == Hooks::GameState::PLACE_QUEST && s.ranged == 1);  // (the bow's coatings: the base unit's)

    // the touch screen: a menu or a window of the game's on it (any interface unit drawn there that is not its panel at
    // rest), which the single screen has the game draw on the main screen (mh3u_onescreen.cpp)
    bool list_on_top = false;    // (a list of the game's drawn on the main screen alone, TOP_LISTS: for s.menu below)
    bool window_on_top = false;  // (a window of the game's there, WINDOWS_ON_TOP: the same)
    {
        u32 top_views = 0, touch_views = 0;
        const u32 camera = g.R32(SCAMERA);
        for (u32 v = 0; Guest::Heap(camera) && v < VIEWPORT_COUNT; ++v) {
            const u32 vp = camera + VIEWPORTS + VIEWPORT_SIZE * v;
            if (g.R8(vp + VIEWPORT_VISIBLE) == 0) continue;
            (g.R8(vp + VIEWPORT_DISPLAY) == 0 ? top_views : touch_views) |= 1u << v;
        }
        views_top = top_views, views_touch = touch_views;
        int menu = 0, place = 0;
        bool always = false, other = false, top_list = false, online = false;
        std::vector<std::size_t> menu_units;  // (the touch menu's units: the single screen draws them on the top screen)
        lobby_top = 0;
        const char* which = nullptr;
        bool town_panel_up = false;  // (the town's panel drawn on the touch screen: REST_WITH_TOWN_PANEL)
        bool making = false;         // (the character's creation on the touch screen: its frame's help line is a menu's)
        bool market = false;         // (the Market's list on the main screen: its item panel is a menu unit, MarketPanel)
        bool facility = false;       // (a town's facility on the main screen: its touch list at the left, Facility)
        bool equipment_shop = false; // (the Smithy's and the Armory's, uIDLobbyEShop: their panel asks no quantity)
        for (const Unit& unit : units) {
            const u32 views = (unit.flags >> VIEW_SHIFT) & 0x3ff;
            if ((unit.flags & UNIT_DRAWN) == UNIT_DRAWN && (views & touch_views) != 0 && (views & top_views) == 0) {
                if (std::string_view(unit.name) == "uIDLobbyTop") town_panel_up = true;
                if (std::string_view(unit.name) == "uIDMakeCharacter") making = true;
            }
            if ((unit.flags & UNIT_DRAWN) == UNIT_DRAWN && (views & top_views) != 0 && (views & touch_views) == 0) {
                const std::string_view n = unit.name;
                if (n == "uIDLobbyMaki") market = s.place == Hooks::GameState::PLACE_TOWN;
                if (n == "uIDLobbyEShop" || n == "uIDLobbyEShopRoot") equipment_shop = true;
            }
            if ((unit.flags & UNIT_DRAWN) == UNIT_DRAWN && (views & top_views) != 0 && Facility(unit.name) &&
                s.place == Hooks::GameState::PLACE_TOWN) {
                facility = true;
            }
        }
        // (Port Tanzia's Smithy draws the same list unit, uIDLobbyMaki, with its own, uIDLobbyEShop: its menus and its
        // Forge list stay on the single screen, the Moto)
        if (equipment_shop) market = false;
        for (const Unit& unit : units) {
            const u32 views = (unit.flags >> VIEW_SHIFT) & 0x3ff;
            if ((unit.flags & UNIT_DRAWN) == UNIT_DRAWN && (views & top_views) != 0 && (views & touch_views) == 0 &&
                TopList(unit.name)) {
                top_list = true;
            }
            if ((unit.flags & UNIT_DRAWN) == UNIT_DRAWN && (views & top_views) != 0 && (views & touch_views) == 0 &&
                WindowOnTop(unit.name)) {
                window_on_top = true;
            }
            if (!Starts(unit.name, "uID") || (unit.flags & UNIT_DRAWN) != UNIT_DRAWN || (views & touch_views) == 0 ||
                (views & top_views) != 0) {
                continue;
            }
            if (std::string_view(unit.name) == "uIDLobbyTop") lobby_top = unit.address;
            // (the title's frame, at rest, holds the help line under the menus: in the character's creation it says what
            // each choice does and asks "Cancel character creation?" over its Yes and No, so it comes with the menu there)
            const bool help_line = making && std::string_view(unit.name) == "uIDStartMenuCommonFrame";
            const bool town_page = s.place == Hooks::GameState::PLACE_TOWN && s.menu == 1 && TownMenuPage(unit.name);
            if ((Rest(unit.name) && !help_line && !MarketPanel(unit.name, market) && !town_page) ||
                (town_panel_up && RestWithTownPanel(unit.name))) {
                continue;
            }
            if (!menu) which = unit.name;
            menu = 1;
            menu_units.push_back(static_cast<std::size_t>(&unit - units.data()));
            if (Always(unit.name)) always = true;
            if (!WithListOnTop(unit.name) && std::string_view(unit.name) != "uIDLobbyOnline" &&
                !MarketPanel(unit.name, market)) {
                other = true;
            }
            if (std::string_view(unit.name) == "uIDLobbyOnline") online = true;
            for (const MenuPlace& m : MENU_PLACES)
                if (m.unit == unit.name) place = m.place;
        }
        list_on_top = top_list;
        if (online && !top_list) other = true;  // (the town's panel goes with a shop's list alone)
        if (menu && place == 0 && !other && (s.menu == 1 || top_list) && s.front_end != 1) place = facility ? 2 : 3;
        // (only while a window of the game's has the buttons, or in the front end: a unit of a panel not known yet
        // never brings the touch screen over a hunt, nor takes the mouse from the camera)
        if (s.menu != 1 && !always) menu = 0;
        s.touch_menu = touch_views != 0 ? menu : -1;
        s.touch_menu_place = place;
        touch_menu_unit = which ? which : "";
        // the single screen: that menu drawn on the top screen by the game (mh3u_onescreen.cpp)
        OneScreen::Step(g, frame, settings, units.data(), units.size(), top_views, menu_units, place,
                        a == "aHarborGuild", s);
        static int place_logged = -1;
        const int place_now = s.touch_menu == 1 ? place : -1;
        if (settings.trace_state && place_now != place_logged) {
            place_logged = place_now;
            Log(fmt::format("touch menu: frame {} {}{}", frame,
                            place_now < 0 ? std::string("none")
                            : place_now == 3 ? std::string("below the main screen's lists (place 3)")
                            : place_now == 4 ? std::string("above the main screen's talk box (place 4)")
                                             : fmt::format("place {}", place_now),
                            which ? fmt::format(", {}", which) : std::string()));
        }
        // the town panel's page (the app's town actions, app_native/town.cpp, as MH4U's cLbMain page numbers): 2 the
        // map, 18 Hunters for Hire, + 100 with the hunter search's row (Guild Cards, Items, Gestures) on top; 0 a menu's
        if (s.place == Hooks::GameState::PLACE_TOWN && touch_views != 0) s.town_panel = menu ? 0 : TownPanel(g);
    }
    // A menu of the game's that holds no window of sID's (its count stays 0): the supply box's and the rewards' lists,
    // their own units on the main screen (TOP_LISTS), the rewards' screen itself (aResult) and a window of the game's on
    // the main screen (WINDOWS_ON_TOP: "Save your progress?"). MH4U's state says the box and the rewards from
    // its HUD (the box's state, the quest's end at the rewards), and its controls follow: so here. Without it the touch
    // controls kept the hunt's layout over the list, with no menu pad, a drag turning the camera instead of the cursor
    // and a long press hiding the controls; the PC's movement keys moved the hunter, not the cursor. (Before the single
    // screen drew the touch screen's menus on the main one, the touch screen came over the list and took the taps.) The
    // quest's Item Pouch and Combo List opened from the touch panels stay as they were: only a finger works them (the
    // hunter waits on them crouched), where the game draws them (touch_menu_taps).
    if (s.menu == 0 && (list_on_top || window_on_top || a == "aResult")) s.menu = 1;
    // the quest's Item Pouch panel, for the app's quick items (mh3u_panels.cpp)
    Panels::Step(g, frame, settings, units.data(), units.size(), s.place == Hooks::GameState::PLACE_QUEST, s);
    if (settings.trace_state >= 2) {
        // (a line when the classes alive or their viewports change, with each one's place, size and draw bits then)
        std::vector<std::string> parts, details;
        for (const Unit& unit : units) {
            if ((unit.flags & UNIT_DRAWN) != UNIT_DRAWN || Starts(unit.name, "uDrawID") || Starts(unit.name, "uAppGUI")) {
                continue;  // (the line: the interface units drawn now)
            }
            parts.push_back(fmt::format("{}@{:03x}", unit.name, (unit.flags >> VIEW_SHIFT) & 0x3ff));
            details.push_back(fmt::format("{}@{:03x}[{:08x} f{:04x} p{:.0f},{:.0f} s{:.2f}]", unit.name,
                                          (unit.flags >> VIEW_SHIFT) & 0x3ff, unit.address, unit.flags & 0xffff,
                                          g.F32(unit.address + 0x40), g.F32(unit.address + 0x44),
                                          g.F32(unit.address + 0x60)));
        }
        std::sort(parts.begin(), parts.end());
        std::sort(details.begin(), details.end());
        std::string sig = area_name + ":";
        for (const std::string& p : parts) sig += " " + p;
        if (sig != units_signature) {
            units_signature = sig;
            std::string line = area_name + ":";
            for (const std::string& d : details) line += " " + d;
            Log(fmt::format("units: frame {} {}", frame, line));
        }
    }
    return s;
}

}  // namespace Port::Mh3u::Game

namespace Port::Mh3u {

// MH3U's local-wireless session (cMH3Session, a nNetwork::Session: the game hosts it in towns and quests alike, and
// every 13 to 17 s it is destroyed and hosted again, nwm::UDS's Unbind, DestroyNetwork, Shutdown, then
// InitializeWithVersion, Bind, the scan and BeginHostingNetwork). Its destructor (0x00554764) stops it on the main
// thread (0x002862f4): its two threads end (they look at their stop flag between 16 ms sleeps, and the main thread
// waits for each to end), then the main thread looks every 17 ms whether the session's work is done (its eight slots
// empty). Two thread ends and one sleep: 40 to 60 ms of the frame, which on the console are one frame of its 30.
// - the two threads' sleeps between their looks at the stop flag: the receiving one (0x0054dc24, nwm::UDS's
//   PullPacket on its bind) and the other one (0x0054de58's loop); their SleepThread(16 ms) returns here
constexpr u32 MH3U_SESSION_RECV_SLEEP_RET = 0x0054dcac;
constexpr u32 MH3U_SESSION_LOOP_SLEEP_RET = 0x0054dfec;
// - the main thread's look whether the session's work is done (0x002862f4's loop: SleepThread(17 ms))
constexpr u32 MH3U_SESSION_STOP_SLEEP_RET = 0x0028636c;
// ... its look comes round every millisecond instead (the loop does its work and looks again: it leaves only once
// the slots are empty, as before)
constexpr std::int64_t MH3U_SESSION_STOP_LOOK_NS = 1000000;

/// guest_sleep.h's g_join_ends_sleep: the session's two threads end at once when the main thread waits for them
bool SessionJoinEndsSleep(std::uint32_t sleep_return) {
    return sleep_return == MH3U_SESSION_RECV_SLEEP_RET || sleep_return == MH3U_SESSION_LOOP_SLEEP_RET;
}

/// guest_sleep.h's g_poll_sleep_filter: the main thread's look at the stopped session every millisecond
std::int64_t SessionPollSleep(std::uint32_t return_address, std::int64_t ns) {
    return return_address == MH3U_SESSION_STOP_SLEEP_RET && ns > MH3U_SESSION_STOP_LOOK_NS ? MH3U_SESSION_STOP_LOOK_NS
                                                                                            : ns;
}

}  // namespace Port::Mh3u
