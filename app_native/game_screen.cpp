// MH3U's single screen (game_screen.h).
#include "game_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "hooks.h"

namespace {

bool g_on = false;
// the fade: 0 hidden .. 1 shown, toward g_want; a change of the game's must last kSettleMs to count (a menu that closes
// as the next opens doesn't blink)
float g_shown = 0.f;
bool g_want = false;
int g_place = 0;
bool g_pending = false;
Uint64 g_pending_since = 0, g_last_ms = 0;
constexpr Uint64 kSettleMs = 60;
constexpr float kFadeMs = 130.f;

}  // namespace

void game_screen_set_on(bool on) {
    if (on != g_on) std::fprintf(stderr, "[screen] MH3U's single screen %s\n", on ? "on: the touch screen over the main one while it has a menu" : "off");
    g_on = on;
    if (!on) g_shown = 0.f, g_want = g_pending = false;
}

bool game_screen_on() {
    return g_on;
}

static bool g_taps = false;  // (game_screen_taps found a menu only a finger works, the last time it was asked)

bool game_screen_shown() {
    return g_on && (g_want || g_shown > 0.f || g_taps);
}

bool game_screen_taps(const SDL_FRect& top, SDL_FRect* rect) {
    g_taps = false;
    if (!g_on || top.w <= 0 || top.h <= 0) return false;
    const Port::Hooks::GameState game = Port::Hooks::CurrentGameState();
    if (game.touch_menu != 1 || game.touch_menu_top != 1 || game.touch_menu_taps != 1) return false;
    // (the console's 400 x 240 in the middle of the main screen's picture, at its pixels' size; on a screen narrower
    // than 5:3, the Anbernic's 4:3, the 400 squeezed to its width, android_main.cpp g_top_aspect)
    const float ky = top.h / 240.f, kx = std::min(ky, top.w / 400.f), s = game.touch_menu_block[0];
    *rect = SDL_FRect{top.x + (top.w - 400.f * kx) * .5f + game.touch_menu_block[1] * kx,
                      top.y + game.touch_menu_block[2] * ky, 320.f * s * kx, 240.f * s * ky};
    g_taps = rect->w > 0 && rect->h > 0;  // (the mouse let free for it, game_screen_shown)
    return g_taps;
}

bool game_screen_view(int w, int h, const SDL_FRect& top, bool allowed, SDL_FRect* rect, float* alpha, float* dim,
                      SDL_FRect* top_out) {
    if (!g_on || w <= 0 || h <= 0) return false;
    const Uint64 now = SDL_GetTicks();
    const float dt = g_last_ms ? static_cast<float>(std::min<Uint64>(now - g_last_ms, 100)) : 0.f;
    g_last_ms = now;
    const Port::Hooks::GameState game = Port::Hooks::CurrentGameState();
    // (the game draws the menu on the top screen itself, mh3n mh3u_onescreen.cpp: nothing to bring over it)
    const bool menu = allowed && game.touch_menu == 1 && game.touch_menu_top != 1;
    if (menu != g_want) {
        if (!g_pending) g_pending = true, g_pending_since = now;
        if (now - g_pending_since >= kSettleMs) {
            g_want = menu, g_pending = false;
            std::fprintf(stderr, "[screen] the touch screen %s (game frame %u)\n",
                         menu ? "over the main one: a menu of the game's is on it" : "hidden again", game.frame);
        }
    } else {
        g_pending = false;
    }
    if (g_want) g_place = game.touch_menu_place;
    g_shown = std::clamp(g_shown + (g_want ? dt : -dt) / kFadeMs, 0.f, 1.f);
    if (g_shown <= 0.f) return false;
    const float tw = top.w > 0 ? top.w : static_cast<float>(w), th = top.h > 0 ? top.h : static_cast<float>(h);
    const float tx = top.w > 0 ? top.x : 0.f, ty = top.h > 0 ? top.y : 0.f;
    float rh, rw, x, y;
    if (g_place == 3) {
        // the game's list is on the main screen, the chosen item's details beside it, often at both its sides (the item
        // box, a shop, the supply box, the armory, the rewards; the house's box takes the main screen's left half and
        // its Store Items a list at the right): no part of it is free in every one of them, on every phone's shape. So
        // while such a menu lasts both screens are shown whole, beside each other (the touch screen at the main one's
        // right) or one above the other, as on the console: whichever shows them larger (side by side on a wide phone,
        // stacked on a 4:3 handheld)
        const float a = tw / th, tb = 4.f / 3.f;  // (the main screen's shape, wider with the wide render; the touch screen's)
        const float gap = std::min(w, h) * .01f;
        const float side = std::min(static_cast<float>(h), (w - gap) / (a + tb));
        const float stack = std::min((h - gap) * .5f, w / a);
        if (top_out != nullptr) {
            SDL_FRect main_to{};
            if (side >= stack) {
                const float x0 = (w - (a + tb) * side - gap) * .5f, y0 = (h - side) * .5f;
                main_to = SDL_FRect{x0, y0, a * side, side};
                rw = tb * side, rh = side, x = x0 + a * side + gap, y = y0;
            } else {
                const float y0 = (h - 2.f * stack - gap) * .5f;
                main_to = SDL_FRect{(w - a * stack) * .5f, y0, a * stack, stack};
                rw = tb * stack, rh = stack, x = (w - rw) * .5f, y = y0 + stack + gap;
            }
            const float k = g_shown * g_shown * (3.f - 2.f * g_shown);  // (eased)
            *top_out = SDL_FRect{tx + (main_to.x - tx) * k, ty + (main_to.y - ty) * k, tw + (main_to.w - tw) * k,
                                 th + (main_to.h - th) * k};
        } else {
            // (a caller that can't move the main screen: below it, in the middle, a third of the height)
            rh = std::max(h * .33f, std::min(th * .5f, h * .5f)), rw = rh * 4.f / 3.f;
            x = (w - rw) * .5f, y = std::min(ty + th, static_cast<float>(h)) - rh;
        }
        *dim = 0.f;
    } else if (g_place == 4) {
        // the game's talk box is at the bottom of the main screen (its name over it: the lowest 30 %, at the left), and
        // an NPC talks there while the touch screen has the menu (the quest counter's card: "Bon voyage!"): the touch
        // screen large above that band, so both are seen; the main screen only a little darker, the talk kept readable
        const float margin = th * .01f;
        rh = std::min(th * .69f - 2.f * margin, static_cast<float>(h) * .80f);
        rw = rh * 4.f / 3.f;
        if (rw > w * .92f) rw = w * .92f, rh = rw * 3.f / 4.f;
        x = (w - rw) * .5f;
        y = ty + margin;
        *dim = .2f;
    } else if (g_place == 1 || g_place == 2) {
        // beside what the main screen shows: at its right (or left) edge, low, about two thirds of its height
        rh = std::min(th * .64f, static_cast<float>(h) * .64f);
        rw = rh * 4.f / 3.f;
        const float margin = th * .02f;
        // (a window not much wider than the main screen's 5:3, a 4:3 handheld's: the game's own list takes the other half
        // of the main screen, so the touch screen keeps to its half)
        if (rw > w * .5f - margin) rw = w * .5f - margin, rh = rw * 3.f / 4.f;
        x = g_place == 1 ? tx + tw - rw - margin : tx + margin;
        y = ty + th - rh - margin;
        *dim = 0.f;
    } else {
        // large in the middle, the main screen a little darker behind it
        rh = std::min(th * .80f, static_cast<float>(h) * .80f);
        rw = rh * 4.f / 3.f;
        if (rw > w * .92f) rw = w * .92f, rh = rw * 3.f / 4.f;
        x = (w - rw) * .5f;
        y = ty + (th - rh) * .5f;
        *dim = .45f;
    }
    const float s = .94f + .06f * g_shown;
    const float cx = x + rw * .5f, cy = y + rh * .5f;
    *rect = SDL_FRect{cx - rw * s * .5f, cy - rh * s * .5f, rw * s, rh * s};
    *alpha = g_shown;
    *dim *= g_shown;
    return true;
}
