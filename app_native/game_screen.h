// MH3U's single screen (mh4u.ini single_screen, Graphics & Audio > Screen and HUD > Single-screen mode), as MH4U's: the
// touch screen is not shown beside the main one, and the game draws the menus it has on the touch screen (the main menu,
// the save's choice, the START menu's buttons, a shop's or the box's OK and Back, the quest counter's lists and card...)
// on the main screen itself, in place, at the main screen's resolution (mh3n port/src/mh3u_onescreen.cpp: their units
// drawn in the top screen's viewport, scaled and moved by the uGUI draw hook; GameState's touch_menu_top). They are
// worked with the buttons, as MH4U's; the touch screen's own picture never comes over the main one. Shown by the player
// (its corner, stacked, side by side, a second display, the town's large screen), the touch screen keeps its menus, as
// on the console (Port::Mh3u::SetTouchScreenShown). The few menus only a finger works (a quest's Item Pouch and Combo
// List, opened from the touch panels or the app's quick items) take a finger where the game draws them
// (game_screen_taps). game_screen_view's older way, the touch screen's picture over the main one, is left for a menu the
// game can't draw there: a page of the quest's touch panels, the Item Pouch's grid the quick items open (its grid is the
// panels' own units, its only menu unit its Back: touch_menu_top 0, mh3n mh3u_onescreen.cpp).
#pragma once
#include <SDL3/SDL.h>

// The game whose single screen this is: MH3U, USA (content.h's content_title())
constexpr unsigned long long kGameScreenTitle = 0x00040000000AE400ull;

// Whether the single screen of this kind runs (the game is MH3U, the option on): set by the program at the start and
// when the option changes (ui_set_one_screen's caller)
void game_screen_set_on(bool on);
bool game_screen_on();

// Each frame, before the screens are drawn: whether the touch screen is shown over the main one now, and where (`rect`,
// window pixels, in a window w x h whose main screen is at `top`), with `alpha` how far it has faded in (0..1) and
// `dim` how much the main screen behind it is darkened (0..1). `allowed`: false while the touch screen shows anyway
// (its corner open, the layout's own place, a window of its own) or another view has it: then it fades out.
// `top_out` (may be null): where the main screen goes meanwhile when it has to make room (a list the game draws on the
// main screen, with no room beside it: the main screen above, the touch screen below, as on the console); left as it
// was when the main screen stays where it is.
bool game_screen_view(int w, int h, const SDL_FRect& top, bool allowed, SDL_FRect* rect, float* alpha, float* dim,
                      SDL_FRect* top_out = nullptr);
// Whether the touch screen is over the main one now (or coming): the mouse is let free for it meanwhile (controls.cpp)
bool game_screen_shown();
// (the game draws the menu on the top screen itself: GameState touch_menu_top, mh3n mh3u_onescreen.cpp) Where a finger
// works that menu as the touch screen (`rect`, window pixels; the main screen at `top`): the touch screen's 320 x 240
// where the game draws it, for the menus only a finger works (GameState touch_menu_taps: a quest's Item Pouch and Combo
// List). False when there is none: the game's menus on the top screen are then worked with the buttons, as MH4U's.
bool game_screen_taps(const SDL_FRect& top, SDL_FRect* rect);
