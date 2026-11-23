// The MH3U app's colours, all in one place: water, as the game's theme and the app's icon (res/mipmap-*/ic_launcher*:
// a deep sea-green plate, #041118 at its corners to #1d5563 at its heart, and a gold ring with gold waves on it, #e2b65c).
// The settings menu, the touch controls and their editor, the chat, the FPS counter, the notices, the town's and the
// weapon tree's panels, the PC's own screens: dark sea-green plates with a steel-blue rim and the icon's gold, the picked
// row lit sea-green to the shallows, the windows' frame two lines of water (deep, then foam) instead of the MH4U's red and
// green. What only says something stays as it was everywhere: on / online green, a warning's coral, an unread mark's red,
// the game's elements, rarities and sharpness, a chat channel's or a name's colour, and a player's own button pictures.
// (Java has the same values: android/.../Look.java)
#pragma once
#include "imgui.h"

namespace pal {
// c with its alpha set to a (0..255): pal::alpha(pal::PANEL, 230)
constexpr ImU32 alpha(ImU32 c, int a) { return (c & ~IM_COL32_A_MASK) | ((ImU32)a << IM_COL32_A_SHIFT); }
// its parts, for SDL's own drawing: SDL_SetRenderDrawColor(r, pal::red(c), pal::green(c), pal::blue(c), 255)
constexpr unsigned char red(ImU32 c) { return (unsigned char)((c >> IM_COL32_R_SHIFT) & 0xff); }
constexpr unsigned char green(ImU32 c) { return (unsigned char)((c >> IM_COL32_G_SHIFT) & 0xff); }
constexpr unsigned char blue(ImU32 c) { return (unsigned char)((c >> IM_COL32_B_SHIFT) & 0xff); }

// ---- the icon's own
constexpr ImU32 ABYSS = IM_COL32(4, 17, 24, 255);     // (its corners)
constexpr ImU32 SEA = IM_COL32(19, 56, 67, 255);      // (between)
constexpr ImU32 TIDE = IM_COL32(29, 85, 99, 255);     // (its heart)
constexpr ImU32 GOLD = IM_COL32(226, 182, 92, 255);   // (the ring and the waves): rules, rims' inner rings, titles, studs

// ---- windows: the menu, its pop-ups and notices, the counters, the town's, the keyboard's, the PC's screens
constexpr ImU32 PANEL = IM_COL32(14, 40, 50, 255);    // a window (the icon's face)
constexpr ImU32 BAND = IM_COL32(22, 64, 78, 255);     // its header band; the keyboard's main keys
constexpr ImU32 POPUP = IM_COL32(17, 48, 59, 255);    // a pop-up over a window
constexpr ImU32 HELP = IM_COL32(19, 58, 70, 255);     // a row's help, opened under it
constexpr ImU32 DIVIDER = IM_COL32(52, 88, 98, 255);  // the lines between a list's rows
constexpr ImU32 RULE_OUT = IM_COL32(26, 116, 140, 255);  // a window's frame: its outer line, deep water (MH4U: red)
constexpr ImU32 RULE_IN = IM_COL32(116, 204, 218, 255);  // and the thin one inside it, the foam (MH4U: green)

// ---- plates: the touch controls, the menu's buttons, the editor's tags
constexpr ImU32 PLATE = IM_COL32(7, 24, 31, 255);       // a control's face
constexpr ImU32 PLATE_LIT = IM_COL32(24, 88, 104, 255); // held, pressed
constexpr ImU32 RIM = IM_COL32(52, 116, 132, 255);      // its rim (steel-blue; MH4U: bronze)
constexpr ImU32 FACE = IM_COL32(16, 50, 61, 255);       // the menu pad's keys, a d-pad's arms
constexpr ImU32 FACE_LIT = IM_COL32(36, 120, 140, 255); // pressed; both of a diagonal's
constexpr ImU32 WELL = IM_COL32(5, 16, 21, 255);        // a dark well: a field to type in, a letter's disc, a switch off
constexpr ImU32 BUTTON = IM_COL32(22, 70, 86, 255);     // a big button on a window (the PC's screens; Java's BUTTON)
constexpr ImU32 BUTTON_LIT = IM_COL32(34, 108, 128, 255);  // under the mouse, the main one

// ---- the picked row, as the game's cursor: lit from the shallows (left) to the deep (right), a gold glow round it
constexpr ImU32 BAR_HI = IM_COL32(42, 134, 154, 255);
constexpr ImU32 BAR_MID = IM_COL32(30, 102, 120, 255);
constexpr ImU32 BAR_LO = IM_COL32(16, 66, 80, 255);
constexpr ImU32 GLOW = IM_COL32(255, 214, 110, 255);    // the picked one's edge and mark, a held control's ring
constexpr ImU32 FOAM = IM_COL32(200, 240, 248, 255);    // a pressed row's wash (faint), a lit tile's edge
constexpr ImU32 SWITCH_ON = IM_COL32(28, 122, 142, 255);  // a switch's slot when on (its stud lights GLOW)
constexpr ImU32 KNOB_OFF = IM_COL32(110, 134, 140, 255);  // its stud when off

// ---- letters
constexpr ImU32 INK = IM_COL32(232, 242, 240, 255);     // text (MH4U: cream)
constexpr ImU32 INK_LIT = IM_COL32(250, 254, 253, 255); // on the picked row
constexpr ImU32 INK_DIM = IM_COL32(160, 192, 198, 255); // a second line, a value, a note
constexpr ImU32 INK_FAINT = IM_COL32(112, 146, 152, 255);
constexpr ImU32 INK_OFF = IM_COL32(98, 126, 132, 255);  // greyed: does nothing now
constexpr ImU32 SILHOUETTE = IM_COL32(60, 86, 94, 255); // the weapon tree's "?" icons

// ---- what says something (kept as the MH4U's: they read the same on any colour)
constexpr ImU32 WARN = IM_COL32(255, 110, 84, 255);     // a warning, an error (coral)
constexpr ImU32 WARN_SOFT = IM_COL32(255, 150, 120, 255);
constexpr ImU32 GOOD = IM_COL32(132, 204, 96, 255);     // fine (the FPS counter)
constexpr ImU32 BAD = IM_COL32(236, 84, 64, 255);       // not fine
constexpr ImU32 ONLINE = IM_COL32(132, 206, 98, 255);   // online, in a room, done
constexpr ImU32 BADGE = IM_COL32(176, 34, 30, 255);     // an unread count's disc
}  // namespace pal
