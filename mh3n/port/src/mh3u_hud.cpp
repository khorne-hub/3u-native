// MH3U (mh3u.h): the hunting HUD's parts moved, sized and hidden (mh4u.ini hud_size and hud_<part>, the app's layout
// editor). Each part of MH3U's HUD is an interface unit of its own (uIDCockpit*, a uGUI): its draw (uGUI's slot 24,
// 0x00549748, hooked by mh3u_gfx.inc) writes the 2D projection of the unit's GUI into four constants before the GUI's
// instance draws with them; that hook asks HudDraw (below) for the unit's part and scales and moves the constants round
// the part's corner, or leaves the unit undrawn when the part is hidden. Written from the frame's step instead, neither
// the GUI instance's root matrix nor the unit's viewport bits held (the game sets them again before it draws: woods1's
// probes). Here, once a frame: whether each part is drawn, and where, for the app's layout editor.
#include "mh3u_game.h"

#include "mh3u_classes.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

namespace Port::Hooks {
void Mh3uHudRows(std::uint32_t* wide, std::uint32_t* edge, float* stretch);  // (hooks.cpp: WideRows, EdgeRows)
}  // namespace Port::Hooks

namespace Port::Mh3u::Hud {

namespace {

constexpr u32 UNIT_FLAGS = 0x04;
constexpr u32 VIEW_BITS = 0x3ffu << 16;

// The parts (Hooks::HudPartKey's order: clock, bars, sharpness, party, map, target, items), each the units of one class
// (nullptr: MH3U's HUD has no such part of its own), its corner on the top screen (fractions: the part grows and shrinks
// toward it, as Heavenfall's smaller HUD), its art at the game's place (fractions of the console's 400 x 240 picture) and
// its side (-1 left, 1 right: where hud_edges takes it on a wide top screen, as mh3u_gfx.inc's Mh3uHudSide)
struct PartDef {
    const char* unit;
    float anchor[2];
    float rect[4];
    int side;
};
constexpr PartDef PARTS[Hooks::HUD_PART_COUNT] = {
    // (the art's boxes as drawn in the Moga Woods at the game's size, woods1's shots)
    {nullptr, {0, 0}, {}, -1},                                         // clock: in the bars' unit
    {"uIDCockpitMain", {0, 0}, {0.0f, 0.0f, 0.70f, 0.40f}, -1},         // the clock, health, stamina, sharpness, the name
    {nullptr, {0, 0}, {}, -1},                                         // sharpness: in the bars' unit
    {"uIDCockpitAbovehead", {0, 0}, {}, 0},                             // the names over the hunters (in the world)
    {"uIDCockpitMap", {1, 0.55f}, {0.76f, 0.44f, 1.0f, 0.64f}, 1},      // the area's map (when not on the touch panels)
    {nullptr, {1, 0}, {}, 1},                                          // target camera: on the touch panels in MH3U
    {"uIDCockpitItem", {1, 1}, {0.74f, 0.70f, 0.94f, 0.93f}, 1},        // the item bar
};

// The area map's art (uIDCockpitMap): a square picture of the stage's map, whose top-left corner, side and scale the
// game keeps in the map block the sign unit reads (uIDCockpitSign's [+ 0x464]: + 0x8c and + 0x8e s16 the corner, + 0x90
// f32 the picture's side, + 0x94 f32 its scale, + 0x98 to + 0xa4 the stage's bounds; 0x0034cb30 puts the hunters' arrows
// and the monsters' icons on it at the corner plus side times scale times their place's fraction of the stage), in the
// console's 400 x 240. Its size and place change with the stage and the hunter's progress (a new hunter's Moga Woods: the
// area round the hunter, small, at the right's middle; with the woods all open, the whole woods, large, from near the top;
// the Deserted Island's higher: the S25 Ultra's editor framed it under the map), so while the map is drawn on the top
// screen its box is that square; else (the map among the touch panels, not drawn yet) the measured box above.
constexpr u32 SIGN_MAP_BLOCK = 0x464;      // uIDCockpitSign: its map block*
constexpr u32 MAP_X = 0x8c, MAP_Y = 0x8e, MAP_SIDE = 0x90, MAP_SCALE = 0x94;
constexpr int HUD_MAP = 4;                 // (Hooks::HudPartKey's order)
float map_box[4] = {};                     // the map's square now (the 400 x 240's pixels), or none (x1 <= x0)
// A bow's coatings and a bowgun's ammo: their panel over the item bar, with the items' part (mh3u_gfx.inc's sides; the
// bow's, the base uIDCockpit's, beside the items: HudGuiDraw), as drawn on the Moto and the RG406V with the Hunter's Bow
// (the 400 x 240's pixels)
constexpr float RANGED_PANEL[4] = {279.0f, 150.0f, 390.0f, 172.0f};
constexpr int HUD_BARS = 1, HUD_ITEMS = 6;
float map_logged[4] = {};

/// The map's square from the sign unit's map block, if it holds one that fits the top screen.
bool MapBox(const Guest& g, u32 sign, float box[4], bool trace) {
    const u32 block = sign != 0 ? g.R32(sign + SIGN_MAP_BLOCK) : 0;
    if (trace) {  // (research: the block as it is, once)
        static bool said = false;
        if (!said) {
            said = true;
            std::string words;
            for (u32 o = 0x80; o < 0xb0 && block != 0; o += 4) words += fmt::format(" {:08x}", g.R32(block + o));
            Log(fmt::format("HUD: the sign unit {:08x}, its map block {:08x}, + 0x80..0xac:{}", sign, block, words));
        }
    }
    if (!Guest::Heap(block)) return false;
    const float x = static_cast<float>(static_cast<std::int16_t>(g.R16(block + MAP_X)));
    const float y = static_cast<float>(static_cast<std::int16_t>(g.R16(block + MAP_Y)));
    const float side = g.F32(block + MAP_SIDE) * g.F32(block + MAP_SCALE);
    if (!(side > 8.0f && side <= 240.0f) || x < -side || x > 400.0f || y < -side || y > 240.0f) return false;
    box[0] = x, box[1] = y, box[2] = x + side, box[3] = y + side;
    return true;
}

std::atomic<bool> in_quest{false};  // (the frame's step: in a quest, where the HUD's sprites are)
std::atomic<bool> bow_hud{false};   // (the frame's step: a quest with the bow, SetBow)

struct Held {
    u32 unit = 0;
    int part = -1;
    u32 seen_frame = 0;
};
std::vector<Held> held;
int seen[Hooks::HUD_PART_COUNT] = {};
u32 shown_frame[Hooks::HUD_PART_COUNT] = {};

int PartOf(std::string_view name) {
    for (int k = 0; k < Hooks::HUD_PART_COUNT; ++k)
        if (PARTS[k].unit != nullptr && name == PARTS[k].unit) return k;
    return -1;
}

}  // namespace

bool InQuest() {
    return in_quest.load(std::memory_order_relaxed);
}

bool Bow() {
    return bow_hud.load(std::memory_order_relaxed);
}

void SetBow(bool bow) {
    bow_hud.store(bow, std::memory_order_relaxed);
}

float PartAnchor(int part, int axis) {
    return part >= 0 && part < Hooks::HUD_PART_COUNT ? PARTS[part].anchor[axis] : 0.0f;
}

const float* PartRect(int part) {
    return part >= 0 && part < Hooks::HUD_PART_COUNT && PARTS[part].unit != nullptr ? PARTS[part].rect : nullptr;
}

void Reset() {
    std::fill(std::begin(map_box), std::end(map_box), 0.0f);
    std::fill(std::begin(map_logged), std::end(map_logged), 0.0f);
    held.clear();
    std::fill(std::begin(seen), std::end(seen), 0);
    std::fill(std::begin(shown_frame), std::end(shown_frame), 0);
}

void Step(const Guest& g, u32 frame, const Settings& settings, const UnitView* units, std::size_t count, bool quest,
          Hooks::GameState& s) {
    in_quest.store(quest, std::memory_order_relaxed);
    // the parts alive now
    for (std::size_t i = 0; i < count; ++i) {
        const int part = PartOf(units[i].name);
        if (part < 0) continue;
        const u32 u = units[i].address;
        auto it = std::find_if(held.begin(), held.end(), [u](const Held& h) { return h.unit == u; });
        if (it == held.end()) {
            Held h;
            h.unit = u, h.part = part;
            held.push_back(h);
            it = held.end() - 1;
        }
        it->seen_frame = frame;
    }
    // each one: the player's place, size and hiding over the game's own
    for (auto it = held.begin(); it != held.end();) {
        Held& h = *it;
        if (h.seen_frame != frame) {  // (the unit is gone: nothing to give back)
            it = held.erase(it);
            continue;
        }
        const int k = h.part;
        const u32 u = h.unit;
        // (the size, the move and the hiding are the draw's: the uGUI draw hook, mh3u_gfx.inc, asks HudDraw below.
        // Written here, once a frame, neither the unit's root matrix nor its viewport bits held: the game sets them
        // again before it draws, woods1's probes)
        (void)k;
        const u32 flags = g.R32(u + UNIT_FLAGS);
        if ((flags & VIEW_BITS) != 0 && (flags & 0x4800) == 0x4800) {  // (drawn: its draw bits too)
            seen[k] = 1;
            shown_frame[k] = frame;
        }
        ++it;
    }
    // the app's view of the parts: fractions of the top screen's picture as it is drawn, which a wide top screen
    // (wide_render) makes wider than the console's 400: the 400 centred in it, and with hud_edges a side's parts at that
    // side's edge (mh3u_gfx.inc: Mh3uGuiDrawOne's and Mh3uSpriteDraw's shift), as MH4U's parts are (hooks.cpp). Up to
    // 1.0.2 they were fractions of the 400 laid over the whole wide picture, with no shift: on the S25 Ultra (520 wide,
    // hud_edges) the bars' frame came out 30 % too wide and the map's and the items' 120 pixels left of their art, and
    // the editor's drag moved a part by 400 / 520 of the finger (the app takes top_width as the picture's width).
    // The map's square while the top screen draws the map (its block may be the touch panels' map's otherwise)
    if (quest && shown_frame[HUD_MAP] != 0 && frame - shown_frame[HUD_MAP] < 30) {
        u32 sign = 0;
        for (std::size_t i = 0; i < count && sign == 0; ++i)
            if (std::string_view(units[i].name) == "uIDCockpitSign") sign = units[i].address;
        float box[4];
        if (MapBox(g, sign, box, settings.trace_state != 0)) {
            std::copy(box, box + 4, map_box);
            float moved = 0.0f;
            for (int i = 0; i < 4; ++i) moved += std::fabs(box[i] - map_logged[i]);
            if (moved >= 1.0f) {
                std::copy(box, box + 4, map_logged);
                Log(fmt::format("HUD: the area map's square at {:.0f}, {:.0f} - {:.0f}, {:.0f} (the 400 x 240's)", box[0], box[1],
                                box[2], box[3]));
            }
        }
    }
    std::uint32_t wide = 0, edge = 0;
    float stretch = 1.0f;
    Hooks::Mh3uHudRows(&wide, &edge, &stretch);
    // (a ranged weapon's panel over the items, drawn now: the base uIDCockpit's coatings with the bow, the bowgun's units)
    bool ranged_panel = quest && Bow();
    for (std::size_t i = 0; i < count && quest && !ranged_panel; ++i) {
        const std::string_view name = units[i].name;
        ranged_panel = (name == "uIDCockpitGunner" || name == "uIDCockpitBallet" || name == "uIDCockpitBow") &&
                       (units[i].flags & VIEW_BITS) != 0 && (units[i].flags & 0x4800) == 0x4800;
    }
    const float width = wide != 0 ? static_cast<float>(wide) : 400.0f;
    s.top_width = static_cast<int>(width);
    const float middle = (width - 400.0f) * 0.5f;
    const float out = edge != 0 ? (static_cast<float>(edge) - 400.0f) * 0.5f : 0.0f;  // (edge 0: hud_edges off)
    for (int k = 0; k < Hooks::HUD_PART_COUNT; ++k) {
        Hooks::GameState::HudPart& p = s.hud[k];
        if (PARTS[k].unit == nullptr || std::string_view(PARTS[k].unit) == "uIDCockpitAbovehead") {
            p.absent = 1;
            continue;
        }
        const float size = std::clamp(settings.hud_size, 0.6f, 1.0f) * settings.hud_k[k];
        p.seen = seen[k];
        p.hidden = settings.hud_hidden[k] ? 1 : 0;
        p.shown = !p.hidden && shown_frame[k] != 0 && frame - shown_frame[k] < 30 && quest ? 1 : 0;
        p.offset[0] = settings.hud_dx[k], p.offset[1] = settings.hud_dy[k];
        p.scale = size;
        // (in the picture's pixels: the part's corner, where it is drawn with its side's shift and the player's move)
        const float ax = middle + static_cast<float>(PARTS[k].side) * out + PARTS[k].anchor[0] * 400.0f + settings.hud_dx[k];
        const float ay = PARTS[k].anchor[1] * 240.0f + settings.hud_dy[k];
        const float ax400 = PARTS[k].anchor[0] * 400.0f, ay240 = PARTS[k].anchor[1] * 240.0f;
        float box[4] = {PARTS[k].rect[0] * 400.0f, PARTS[k].rect[1] * 240.0f, PARTS[k].rect[2] * 400.0f,
                        PARTS[k].rect[3] * 240.0f};
        if (k == HUD_MAP && map_box[2] > map_box[0]) std::copy(map_box, map_box + 4, box);
        if (k == HUD_ITEMS && ranged_panel) {
            box[0] = std::min(box[0], RANGED_PANEL[0]), box[1] = std::min(box[1], RANGED_PANEL[1]);
            box[2] = std::max(box[2], RANGED_PANEL[2]), box[3] = std::max(box[3], RANGED_PANEL[3]);
        }
        float x0 = ax + (box[0] - ax400) * size, x1 = ax + (box[2] - ax400) * size, anchor = ax;
        if (k == HUD_BARS && stretch != 1.0f) {
            // (the bars' part - the clock and the gauges, the uDrawIDSprite units, and the hunters' name plates - is drawn
            // through the main view's projection; Mh3uSpriteUnscaled takes its widening off, so Mh3uHudRows gives 1 here.
            // With mh3u_hud_unscale = off it is drawn widened from the picture's middle, as before (at 4:3 a quarter
            // wider, the clock off the screen's left): its frame where it is drawn, within the picture)
            const float mid = width * 0.5f;
            x0 = mid + (x0 - mid) * stretch, x1 = mid + (x1 - mid) * stretch, anchor = mid + (anchor - mid) * stretch;
            x0 = std::max(x0, 0.0f), x1 = std::min(x1, width);
        }
        p.rect[0] = x0 / width;
        p.rect[1] = (ay + (box[1] - ay240) * size) / 240.0f;
        p.rect[2] = x1 / width;
        p.rect[3] = (ay + (box[3] - ay240) * size) / 240.0f;
        p.anchor[0] = anchor / width, p.anchor[1] = ay / 240.0f;
    }
    if (settings.trace_state) {  // (tests: the parts' frames on the runner's screenshots; a line when one changes)
        static std::string logged;
        std::string line = fmt::format("HUD parts (picture {:.0f} wide):", width);
        for (const int k : {1, 4, 6}) {
            const Hooks::GameState::HudPart& p = s.hud[k];
            line += fmt::format(" {} {} {:.4f},{:.4f},{:.4f},{:.4f};", Hooks::HudPartKey(k), p.shown, p.rect[0], p.rect[1],
                                p.rect[2], p.rect[3]);
        }
        if (line != logged) logged = line, Log(fmt::format("frame {} {}", frame, line));
    }
}

}  // namespace Port::Mh3u::Hud

namespace Port::Mh3u {

namespace {

// The HUD's units by part (the weapons' own gauges go with the bars at the left, a bowgun's ammo with the items at the
// right, as mh3u_gfx.inc's sides): Hooks::HudPartKey's order. The bow's coatings' panel is the base uIDCockpit's
// (HudGuiDraw)
struct DrawPart {
    const char* unit;
    int part;
};
// The area map's part is two units: the map (uIDCockpitMap) and the maps' signs (uIDCockpitSign: the hunters' arrows,
// the painted monsters' icons, the camp's), which its step places on the map from the map's place and scale as the map
// leaves them in sID's map block (sID + 0x36c: + 0x8c, + 0x8e, + 0x94; 0x0034c08c reads them) and draws as a unit of its
// own, on both screens (the touch panels' map too). Drawn with the map's part (its size, move and hiding on the top
// screen), the hunter's arrow stays on the map wherever the player puts it. The same unit draws the hunters' names on
// their plates over the bars (mh3u-pc2's run a2: hiding the unit hid the name): its nodes (unit + 0x1f8, 36) are the
// names' or the map's (SignNameNodes), and mh3u_gfx.inc draws the unit twice on the top screen, the names with the bars'
// part and side, the rest with the map's (SetSignPass).
constexpr DrawPart DRAW_PARTS[] = {
    {"uIDCockpitMain", 1}, {"uIDCockpitAir", 1}, {"uIDCockpitAxe", 1}, {"uIDCockpitWsword", 1},
    {"uIDCockpitGunLance", 1}, {"uIDCockpitPipe", 1}, {"uIDCockpitBow", 6},
    {"uIDCockpitMap", 4}, {"uIDCockpitSign", 4},
    {"uIDCockpitItem", 6}, {"uIDCockpitGunner", 6}, {"uIDCockpitBallet", 6},
};

// (research, mh3u_sign = <part> <side>: MH4N_SET_LIVE / the runner's --set) the signs with the map's part (1, the
// default) or as the game draws them (0); on the wide top screen at the map's side (1, the default) or centred (0)
std::atomic<int> sign_part{1}, sign_side{1};
// the sign unit's nodes that are the hunters' names (bit i: node i), the draw of the sign unit going on (0 the whole
// unit, 1 its names, 2 the rest), and (research) the nodes to dump once and to hide
std::atomic<std::uint64_t> sign_names{kSignNameNodes}, sign_hide{0};
std::atomic<int> sign_pass{0}, sign_dump{0};

int DrawPartOf(std::uint32_t vtable) {  // (-1: no part)
    const char* name = nullptr;
    for (const ClassVtable& c : kClassVtables)
        if (c.vtable == vtable) name = c.name;
    if (name == nullptr) return -1;
    if (std::string_view(name) == "uIDCockpitSign") {
        if (sign_pass.load(std::memory_order_relaxed) == 1) return 1;  // (its names: with the bars)
        if (sign_part.load(std::memory_order_relaxed) == 0) return -1;
    }
    for (const DrawPart& d : DRAW_PARTS)
        if (std::string_view(name) == d.unit) return d.part;
    return -1;
}

}  // namespace

namespace {

bool PartDraw(int k, float* size, float* dx, float* dy, float* anchor_x, float* anchor_y, bool* hidden) {
    const Settings& settings = CurrentSettings();
    *size = std::clamp(settings.hud_size, 0.6f, 1.0f) * settings.hud_k[k];
    *dx = settings.hud_dx[k], *dy = settings.hud_dy[k];
    *anchor_x = Hud::PartAnchor(k, 0) * 400.0f, *anchor_y = Hud::PartAnchor(k, 1) * 240.0f;
    *hidden = settings.hud_hidden[k];
    return *hidden || std::fabs(*size - 1.0f) > 1e-3f || *dx != 0 || *dy != 0;
}

}  // namespace

bool SetSignProbe(const std::string& value) {
    int part = 1, side = 1;
    unsigned long long names = kSignNameNodes, hide = 0;
    int dump = 0;
    if (std::sscanf(value.c_str(), "%d %d %llx %llx %d", &part, &side, &names, &hide, &dump) < 1) return false;
    sign_part.store(part, std::memory_order_relaxed), sign_side.store(side, std::memory_order_relaxed);
    sign_names.store(names, std::memory_order_relaxed), sign_hide.store(hide, std::memory_order_relaxed);
    sign_dump.store(dump, std::memory_order_relaxed);
    Log(fmt::format("the maps' signs: {} the map's part, {} on the wide top screen; names {:x}, hidden {:x}{}",
                    part ? "with" : "without", side ? "at the map's side" : "centred", names, hide,
                    dump ? ", dumped once" : ""));
    return true;
}

bool SignAtMapSide() {
    if (sign_pass.load(std::memory_order_relaxed) == 1) return false;  // (its names: at the bars' side, SignNamePass)
    return sign_side.load(std::memory_order_relaxed) != 0;
}

std::uint64_t SignNameNodes() {
    return sign_part.load(std::memory_order_relaxed) == 0 ? 0 : sign_names.load(std::memory_order_relaxed);
}

std::uint64_t SignHiddenNodes() {
    return sign_hide.load(std::memory_order_relaxed);
}

bool SignDumpTake() {
    return sign_dump.exchange(0, std::memory_order_relaxed) != 0;
}

void SetSignPass(int pass) {
    sign_pass.store(pass, std::memory_order_relaxed);
}

bool SignNamePass() {
    return sign_pass.load(std::memory_order_relaxed) == 1;
}

bool HudDraw(std::uint32_t vtable, float* size, float* dx, float* dy, float* anchor_x, float* anchor_y, bool* hidden) {
    if (!Active()) return false;
    const int k = DrawPartOf(vtable);
    return k >= 0 && PartDraw(k, size, dx, dy, anchor_x, anchor_y, hidden);
}

namespace {

constexpr std::uint32_t kCockpitVtable = 0x00b4f038;  // uIDCockpit (mh3u_classes.h)

const char* ClassName(std::uint32_t vtable) {
    for (const ClassVtable& c : kClassVtables)
        if (c.vtable == vtable) return c.name;
    return "?";
}

}  // namespace

bool HudGuiDraw(std::uint32_t unit, std::uint32_t vtable, bool placed, float tx, float ty, float* size, float* dx, float* dy,
                float* anchor_x, float* anchor_y, bool* hidden) {
    if (!Active()) return false;
    int k = DrawPartOf(vtable);
    const bool quest = Hud::InQuest();
    if (k < 0 && vtable == kCockpitVtable && quest && Hud::Bow()) k = 6;  // (the bow's coatings, beside the items)
    if (quest && CurrentSettings().trace_state) {  // (once a unit and part: what the top screen of a quest draws)
        static std::mutex m;
        static std::vector<std::uint64_t> seen_units;
        const std::uint64_t key = std::uint64_t{unit} << 8 | static_cast<std::uint8_t>(k + 1);
        std::lock_guard lock(m);
        if (seen_units.size() < 256 && std::find(seen_units.begin(), seen_units.end(), key) == seen_units.end()) {
            seen_units.push_back(key);
            Log(fmt::format("HUD unit {:08x} {} (vtable {:08x}): {}{:.1f}, {:.1f}; {}", unit, ClassName(vtable), vtable,
                            placed ? "its GUI at " : "no GUI instance, ", tx, ty,
                            k >= 0 ? fmt::format("part {}", k) : std::string("no part's")));
        }
    }
    return k >= 0 && PartDraw(k, size, dx, dy, anchor_x, anchor_y, hidden);
}

void HudSpriteSeen(std::uint32_t unit, bool placed, float x, float y, float w, float h, std::uint32_t elements, bool part) {
    static std::mutex m;
    static std::vector<std::uint64_t> seen_units;  // (the unit and what was told of it)
    if (!Active() || !CurrentSettings().trace_state) return;
    const std::uint64_t key = std::uint64_t{unit} << 8 | (placed ? 1u : 0u) | (part ? 2u : 0u) | (Hud::InQuest() ? 4u : 0u);
    std::lock_guard lock(m);
    if (seen_units.size() >= 128 || std::find(seen_units.begin(), seen_units.end(), key) != seen_units.end()) return;
    seen_units.push_back(key);
    if (placed)
        Log(fmt::format("HUD sprite {:08x}: its elements' box at {:.0f}, {:.0f}, {:.0f} x {:.0f}, {} elements; {}{}", unit, x,
                        y, w, h, elements, part ? "a HUD part's" : "no part's", Hud::InQuest() ? " (quest)" : ""));
    else
        Log(fmt::format("HUD sprite {:08x}: no element of its first list drawn ({}), at {:.0f}, {:.0f}; {}{}", unit, elements, x,
                        y, part ? "a HUD part's" : "no part's", Hud::InQuest() ? " (quest)" : ""));
}

bool HudInQuest() {
    return Hud::InQuest();
}

bool HudBow() {
    return Hud::InQuest() && Hud::Bow();
}

bool HudSprite(bool placed, float x, float y, float* size, float* dx, float* dy, float* anchor_x, float* anchor_y,
               bool* hidden) {
    if (!Active() || !Hud::InQuest()) return false;
    // In a quest the HUD's sprites are the bars' (the clock, the health, stamina and sharpness gauges: woods4, where every
    // sprite unit drawn moved with them), but for one whose drawn elements fall in another part's box
    const float u = x / 400.0f, v = y / 240.0f;
    for (const int k : {4}) {
        const float* r = Hud::PartRect(k);
        if (placed && r != nullptr && u >= r[0] && u < r[2] && v >= r[1] && v < r[3])
            return PartDraw(k, size, dx, dy, anchor_x, anchor_y, hidden);
    }
    return PartDraw(1, size, dx, dy, anchor_x, anchor_y, hidden);
}

}  // namespace Port::Mh3u
