// MH3U (mh3u.h): the quest's touch panels as the app's quick items see them (GameState items_slot, touch_items_slot), from
// the game's memory once a frame, with no hook point.
//
// In the field (the Moga Woods, a quest) MH3U's touch screen is a grid of panels the player lays out (Customize panels):
// two columns of three slots under the touch screen's bar, the left column's at x 2 and the right one's at x 160, the rows
// at y 39, 106 and 173 (67 pixels apart; MH4U's are 160 x 68 under 36). The unit that holds them, uIDPanelModule (drawn
// on the touch screen's viewport while the panels are up), keeps one record per panel shown, 0x90 bytes each from +0x478:
// its type in the second byte of the u32 at +0x38 and its slot's top-left corner at +0x40, +0x44 (f32). Seen on the
// RG406V in the Moga Woods with the new hunter's layout (anb field2, frames 9000 and 12000; in
// the port's test scripts): 0x0f Signal A and B (2, 39), 0x0b the Combo List (2, 106), 0x0a the Item Pouch
// (2, 173), 0x01 the D-pad for the camera (160, 39; two slots tall), 0x0d the target's (160, 173). A record with no type
// (0) ends the list.
//
// And whether the hunter is under water (GameState underwater): the air gauge's unit is alive.
//
// The quick items' piece (the app's quick_items.cpp) shows the Item Pouch panel's slot on the main screen while the
// panels are up and no menu is on the touch screen (GameState's touch_menu, mh3u_game.cpp): a tap there opens the pouch on
// the touch screen, which the single screen (game_screen.h) brings over the main one, and a tap on an item uses it. MH4U's
// panel of shortcuts has no like in MH3U: its pouch panel is the touch screen's way to an item.
#include "mh3u_game.h"

#include <string_view>

#include <fmt/format.h>

namespace Port::Mh3u::Panels {

namespace {

constexpr u32 MODULE_RECORDS = 0x478, RECORD_SIZE = 0x90, RECORD_COUNT = 6;
constexpr u32 RECORD_TYPE = 0x38, RECORD_X = 0x40, RECORD_Y = 0x44;
constexpr int TYPE_ITEM_POUCH = 0x0a;
constexpr float SLOT_TOP = 39.0f, SLOT_STEP = 67.0f, COLUMN_SPLIT = 80.0f;

// (as MH4U's QuestOverlayChoice, by frames: a slot that comes shows after SETTLE frames, one that goes stays HOLD frames,
// so that a frame between a menu and the panels, or a change of area, doesn't blink the piece)
constexpr u32 SETTLE = 3, HOLD = 30;
struct Choice {
    int shown = -1, pending = -1;
    u32 since = 0;
    void Update(u32 frame, int now) {
        if (now == shown) {
            pending = shown, since = frame;
            return;
        }
        if (now != pending) pending = now, since = frame;
        if (frame - since >= (now < 0 ? HOLD : SETTLE)) shown = now;
    }
};
Choice touch_choice;
int logged = -2;

}  // namespace

void Reset() {
    touch_choice = {};
    logged = -2;
}

void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, bool quest,
          Hooks::GameState& s) {
    // A big panel of the field's (the Item Pouch's grid, "Touch the item you want to use", opened from its panel; the
    // Combo List's) is drawn by the panels' unit itself, with its Back button: a uIDPanelButton drawn on the touch screen,
    // which the panels at rest don't draw (the RG406V's trace of the units, frames 8234 and 8523). The hunter waits
    // crouched until it closes, so it is a menu on the touch screen (GameState touch_menu): the single screen shows it
    // over the main one, large, and the quick items' piece goes meanwhile.
    if (quest && s.touch_menu == 0) {
        for (std::size_t i = 0; i < count; ++i) {
            const u32 flags = units[i].flags;
            if (std::string_view(units[i].name) == "uIDPanelButton" && (flags & 0x4800) == 0x4800 &&
                ((flags >> 16) & 0x3ff & 2) != 0) {
                s.touch_menu = 1, s.touch_menu_place = 0;
                break;
            }
        }
    }
    int slot = -1;
    if (quest && s.touch_menu != 1) {
        for (std::size_t i = 0; i < count && slot < 0; ++i) {
            if (std::string_view(units[i].name) != "uIDPanelModule" || ((units[i].flags >> 16) & 0x3ff) == 0) continue;
            for (u32 r = 0; r < RECORD_COUNT; ++r) {
                const u32 rec = units[i].address + MODULE_RECORDS + RECORD_SIZE * r;
                const int type = static_cast<int>(g.R32(rec + RECORD_TYPE) >> 8 & 0xff);
                if (type == 0) break;
                if (type != TYPE_ITEM_POUCH) continue;
                const float x = g.F32(rec + RECORD_X), y = g.F32(rec + RECORD_Y);
                const int row = static_cast<int>((y - SLOT_TOP) / SLOT_STEP + 0.5f);
                if (row < 0 || row > 2) break;
                slot = (x >= COLUMN_SPLIT ? 3 : 0) + row;
                break;
            }
        }
    }
    touch_choice.Update(frame, slot);
    // Under water (GameState underwater): the air gauge's unit (uIDCockpitAir) is alive and drawn. It is not among the
    // cockpit's units on land (the Moga Woods' camp and areas 1 and 2, frames 9000 and 12000 of the RG406V's run): the
    // game makes it as the hunter dives. Not yet seen under water (no save at a dive yet): -1 out of the field.
    if (quest && s.underwater < 0) {
        s.underwater = 0;
        for (std::size_t i = 0; i < count; ++i)
            if (std::string_view(units[i].name) == "uIDCockpitAir" && ((units[i].flags >> 16) & 0x3ff) != 0) s.underwater = 1;
    }
    s.touch_items_slot = touch_choice.shown;
    s.touch_items_wide = 0;
    s.items_slot = settings.quick_items ? touch_choice.shown : -1;
    s.items_wide = 0;
    // The item bar's place (GameState item_bar: the app's touch zone on it and its arrows beside it): the HUD's item bar
    // part where Hud::Step has it this frame (uIDCockpitItem: s.hud[6], at the player's place and size), in the field
    constexpr int HUD_ITEMS = 6;  // (hooks.cpp's HudPartKey order)
    if (quest) {
        const Hooks::GameState::HudPart& bar = s.hud[HUD_ITEMS];
        s.item_bar_shown = bar.shown == 1 && bar.rect[2] > bar.rect[0] ? 1 : 0;
        if (s.item_bar_shown == 1)
            for (int i = 0; i < 4; ++i) s.item_bar[i] = bar.rect[i];
    }
    static int water_logged = -2;
    if (settings.trace_state && s.underwater != water_logged) {
        water_logged = s.underwater;
        Log(fmt::format("under water: frame {} {}", frame, s.underwater == 1 ? "the air gauge is up" : s.underwater == 0 ? "on land" : "-"));
    }
    if (settings.trace_state && s.touch_items_slot != logged) {
        logged = s.touch_items_slot;
        Log(fmt::format("quick items: frame {} the Item Pouch panel {}", frame,
                        logged < 0 ? std::string("not shown") : fmt::format("in slot {}", logged)));
    }
}

}  // namespace Port::Mh3u::Panels
