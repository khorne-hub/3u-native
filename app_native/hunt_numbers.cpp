// Damage numbers over the top screen (hunt_numbers.h).
#include "hunt_numbers.h"
#include "app_palette.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

#include <string>

#include "app_settings.h"
#include "hunt_aids.h"
#include "imgui.h"

namespace {

// of the top screen's height, in whole pixels: one size for every number (Dear ImGui bakes the font at each size it is
// asked for, so a size that changed every frame would bake new glyphs every frame). The player picks one of three
// ([Graphics] DamageNumberSize: small, medium, large; medium the size they came with) and a colour
// ([Graphics] DamageNumberColour: white, yellow, red, blue, green), read again every half second, so a change in the
// menu shows at once without a file read each frame.
constexpr float kTextHeight = .075f;
constexpr float kSizes[] = {.04f, kTextHeight, .1f};  // (Small was 5.5 %: "still a bit big", the owner on a Galaxy)
constexpr ImU32 kColours[] = {IM_COL32(255, 255, 255, 0), IM_COL32(255, 214, 40, 0), IM_COL32(255, 80, 64, 0),
                              IM_COL32(110, 200, 255, 0), IM_COL32(110, 230, 110, 0)};
// By hit (MH3U: [Graphics] DamageNumberColour = byhit, as the MHGU port's): each hit's own, as the game's damage step
// worked it out (mh4n's HuntAids::Number kind): a weak point orange, a normal hit white, a hard part (a bounce) grey,
// fixed damage (bombs, shells) blue-green, a drop with no hit read for it (poison, another hunter's) white; a critical
// hit a red spark at the top right, a feeble one a blue line under it; the element's damage a second, smaller number in
// its colour (fire, water, thunder, ice, dragon)
constexpr int kColourByHit = 5;
constexpr ImU32 kKindColours[] = {IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0), IM_COL32(255, 168, 36, 0),
                                  IM_COL32(150, 150, 150, 0), IM_COL32(64, 214, 196, 0)};
constexpr ImU32 kElementColours[] = {IM_COL32(220, 220, 220, 0), IM_COL32(255, 104, 56, 0), IM_COL32(72, 156, 255, 0),
                                     IM_COL32(255, 224, 64, 0), IM_COL32(170, 236, 255, 0), IM_COL32(196, 104, 240, 0)};
constexpr ImU32 kSpark = IM_COL32(236, 40, 36, 0), kFeeble = IM_COL32(72, 128, 255, 0);
struct Style {
    float height = kTextHeight;
    ImU32 colour = kColours[0];
    bool by_hit = false;  // (DamageNumberColour = byhit, where the game says what each hit was)
    int style = kStyleClassic;  // ([Graphics] DamageNumberStyle)
};

int key_index(const std::string& v, std::initializer_list<const char*> keys, int fallback) {
    int i = 0;
    for (const char* k : keys) {
        if (v == k) return i;
        ++i;
    }
    return fallback;
}

const Style& style(std::int64_t now_ns) {
    static Style s;
    static std::int64_t next_ns = 0;
    if (now_ns >= next_ns) {
        next_ns = now_ns + 500000000;
        s.height = kSizes[key_index(app_setting("Graphics", "DamageNumberSize", "medium"),
                                    {"small", "medium", "large"}, 1)];
        const int colour = key_index(app_setting("Graphics", "DamageNumberColour", "white"),
                                     {"white", "yellow", "red", "blue", "green", "byhit"}, 0);
        s.by_hit = colour == kColourByHit && Port::HuntAids::HitKindsKnown();
        s.colour = kColours[colour == kColourByHit ? 0 : colour];
        s.style = key_index(app_setting("Graphics", "DamageNumberStyle", "classic"),
                            {"classic", "outline", "compact", "burst"}, kStyleClassic);
    }
    return s;
}
constexpr float kRise = .08f;          // how far it rises while it lasts, of the screen's height
constexpr float kFade = .6f;           // it fades from 60% of its time on
constexpr float kJitter = .006f;       // numbers side by side: up to 4 steps of this (of the width) apart, by their id
constexpr double kCompactSeconds = .7;  // (Compact: its numbers last this long, and rise half as far)
constexpr std::int64_t kJumpNs = 120000000;  // (Outlined and Burst: a number larger for its first 0.12 s)

std::int64_t steady_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

std::vector<HuntNumberDrawn> hunt_numbers_layout(const SDL_FRect& top, std::int64_t now_ns, float text_height, int style) {
    std::vector<HuntNumberDrawn> out;
    if (top.w <= 0 || top.h <= 0) return out;
    // (the sizes: a few fixed steps of the player's, never one that changes every frame: each is baked once)
    const float base = std::max(8.f, std::floor(text_height * top.h + .5f));
    auto step = [&](float k) { return std::max(8.f, std::floor(base * k + .5f)); };
    const float size = style == kStyleCompact ? step(.72f) : style == kStyleBurst ? step(1.1f) : base;
    const float big = step(style == kStyleBurst ? 1.3f : 1.25f);
    const double life = style == kStyleCompact ? kCompactSeconds : Port::HuntAids::NumberSeconds();
    const float rise_k = style == kStyleCompact ? .5f : style == kStyleOutlined ? .8f : style == kStyleBurst ? .15f : 1.f;
    for (const Port::HuntAids::Number& n : Port::HuntAids::Numbers()) {
        if (!n.shown) continue;
        const std::int64_t since = style == kStyleBurst ? n.last_ns : n.born_ns;  // (a run lasts from its last hit)
        float t = (float)((double)(now_ns - since) / 1e9 / life);
        if (t > 1) continue;
        t = std::max(t, 0.f);
        const float age = std::clamp((float)((double)(now_ns - n.born_ns) / 1e9 / life), 0.f, 1.f);
        const float rise = kRise * rise_k * top.h * (1 - (1 - age) * (1 - age));  // (eased: fast at first)
        const float jitter = style == kStyleBurst ? 0.f : (float)((int)(n.id * 37u % 9u) - 4) * kJitter * top.w;
        HuntNumberDrawn d;
        d.x = top.x + n.x * top.w + jitter;
        d.y = top.y + n.y * top.h - rise;
        d.size = (style == kStyleOutlined || style == kStyleBurst) && now_ns - since < kJumpNs ? big : size;
        d.alpha = t < kFade ? 1 : 1 - (t - kFade) / (1 - kFade);
        d.value = n.value;
        d.hits = n.hits;
        d.elem = n.elem;
        d.kind = n.kind;
        d.crit = n.crit;
        d.element = n.element;
        out.push_back(d);
    }
    return out;
}

void hunt_numbers_draw(const SDL_FRect& top) {
    if (!Port::HuntAids::DamageNumbersOn()) return;
    const std::int64_t now = steady_ns();
    const Style& look = style(now);
    const std::vector<HuntNumberDrawn> numbers = hunt_numbers_layout(top, now, look.height, look.style);
    if (numbers.empty()) return;
    ImFont* font = ImGui::GetFont();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();  // (under the touch controls and the menus, as the FPS counter)
    dl->PushClipRect(ImVec2(top.x, top.y), ImVec2(top.x + top.w, top.y + top.h), true);
    for (const HuntNumberDrawn& d : numbers) {
        char text[16];
        std::snprintf(text, sizeof text, "%d", look.by_hit ? d.value : d.value + d.elem);
        const ImVec2 size = font->CalcTextSizeA(d.size, 1e9f, 0, text);
        const ImVec2 at(d.x - size.x * .5f, d.y - size.y * .5f);
        const int a = (int)(255 * std::clamp(d.alpha, 0.f, 1.f));
        // the outline, as the game's own white numbers on dark (Outlined: thicker, with a second ring of shadow)
        const float o = std::max(1.f, d.size * (look.style == kStyleOutlined ? .13f : look.style == kStyleCompact ? .06f : .07f));
        static const float DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1}, DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        if (look.style == kStyleOutlined)
            for (int k = 0; k < 8; ++k)
                dl->AddText(font, d.size, ImVec2(at.x + o * .5f + DX[k] * o, at.y + o * .5f + DY[k] * o),
                            IM_COL32(0, 0, 0, a * 3 / 8), text);
        for (int k = 0; k < 8; ++k) {
            dl->AddText(font, d.size, ImVec2(at.x + DX[k] * o, at.y + DY[k] * o), IM_COL32(0, 0, 0, a * 3 / 4), text);
        }
        const ImU32 ink = look.by_hit ? kKindColours[std::min<int>(d.kind, 4)] : look.colour;
        dl->AddText(font, d.size, at, ink | IM_COL32(0, 0, 0, a), text);
        if (look.by_hit && d.elem > 0) {  // the element's part: smaller, in its colour (Compact: beside it)
            char e[16];
            std::snprintf(e, sizeof e, look.style == kStyleCompact ? "+%d" : "%d", d.elem);
            const float es_size = std::max(8.f, std::floor(d.size * (look.style == kStyleCompact ? .7f : .6f) + .5f));
            const ImVec2 es = font->CalcTextSizeA(es_size, 1e9f, 0, e);
            const ImVec2 ea = look.style == kStyleCompact ? ImVec2(at.x + size.x + es_size * .15f, at.y + size.y - es.y)
                                                          : ImVec2(at.x + size.x - es.x * .3f, at.y + size.y * .78f);
            const float eo = std::max(1.f, es_size * .08f);
            for (int k = 0; k < 8; ++k)
                dl->AddText(font, es_size, ImVec2(ea.x + DX[k] * eo, ea.y + DY[k] * eo), IM_COL32(0, 0, 0, a * 3 / 4), e);
            dl->AddText(font, es_size, ea, kElementColours[std::min<int>(d.element, 5)] | IM_COL32(0, 0, 0, a), e);
        }
        if (look.by_hit && d.crit > 0) {  // a critical hit: a red spark at the top right
            const float r = d.size * .2f, cx = at.x + size.x + r * .6f, cy = at.y + r * .9f, w = std::max(1.5f, d.size * .07f);
            for (int pass = 0; pass < 2; pass++) {  // (its dark edge first, then the spark)
                const float k = pass ? 1.f : 1.3f, ww = pass ? w : w + 2;
                const ImU32 cc = pass ? (kSpark | IM_COL32(0, 0, 0, a)) : IM_COL32(0, 0, 0, a * 3 / 4);
                dl->AddLine(ImVec2(cx - r * k, cy), ImVec2(cx + r * k, cy), cc, ww);
                dl->AddLine(ImVec2(cx, cy - r * k), ImVec2(cx, cy + r * k), cc, ww);
                dl->AddLine(ImVec2(cx - r * .6f * k, cy - r * .6f * k), ImVec2(cx + r * .6f * k, cy + r * .6f * k), cc, ww * .7f);
                dl->AddLine(ImVec2(cx - r * .6f * k, cy + r * .6f * k), ImVec2(cx + r * .6f * k, cy - r * .6f * k), cc, ww * .7f);
            }
        } else if (look.by_hit && d.crit < 0) {  // a feeble one: a blue line under it
            const float y = at.y + size.y * .98f, w = std::max(2.f, d.size * .08f);
            dl->AddLine(ImVec2(at.x, y), ImVec2(at.x + size.x, y), IM_COL32(0, 0, 0, a * 3 / 4), w + 2);
            dl->AddLine(ImVec2(at.x, y), ImVec2(at.x + size.x, y), kFeeble | IM_COL32(0, 0, 0, a), w);
        }
        if (look.style == kStyleBurst && d.hits > 1) {  // how many hits the run holds
            char k[16];
            std::snprintf(k, sizeof k, "x%d", d.hits);
            const float ks = std::max(8.f, std::floor(d.size * .42f + .5f)), ko = std::max(1.f, ks * .08f);
            const ImVec2 kat(at.x + size.x + ks * .25f, at.y + size.y - ks * 1.05f);
            for (int j = 0; j < 8; ++j)
                dl->AddText(font, ks, ImVec2(kat.x + DX[j] * ko, kat.y + DY[j] * ko), IM_COL32(0, 0, 0, a * 3 / 4), k);
            dl->AddText(font, ks, kat, pal::alpha(pal::INK, a), k);
        }
    }
    dl->PopClipRect();
}
