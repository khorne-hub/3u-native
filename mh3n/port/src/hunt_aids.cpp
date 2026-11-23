// Hunting aids of the port (1.0.1), both off unless the player turns them on (hunt_aids.h).
//
// Large monsters always on the map (monsters_on_map). The game shows a large monster on its maps (the touch screen's
// and the top screen's area map) while the monster is painted: its paint timer, a float at the monster's status block
// + 0x20c, is above 0 (0x0095541c; the maps' code, 0x00b8668c and 0x00b86d38, asks it of each large monster, after
// the HUD's own mask: the Autotracker skill, 0x00b964cc). A paintball sets it, and the game sets it itself (36000) at
// the start for the monsters it always shows (0x00910678); the monster's step takes the frame's time off it
// (0x009171a8). Here, in a quest, each large monster that is alive and not painted gets a timer of PAINT_KEEP, kept
// between PAINT_LOW and that once a frame: the game then shows it on both maps, in its place and with its own icon,
// from the quest's start. It stays above 7200, under which a Palico says the paint is wearing off (0x00985edc). A
// monster the player paints (a timer above that) is left to the game; with the option turned off during a quest, a
// timer of ours goes back to 0. Nothing else of the monster's changes. Only in a quest this console hunts alone: with
// other hunters in the quest (a room's) the paint is left to the game, since a painted monster is shown to the whole
// party (a paintball's mark goes to everyone) and it was not proven that a timer written here stays on this console.
//
// Damage numbers (damage_numbers). Once a frame, each monster's health, large and small (1.0.2: a small one has a status
// block of the same layout; s32 at + 0x3e8, its maximum at + 0x3ec, 0x0091ad08), is compared with the last frame's: a
// drop is a number (with damage_number_burst, the app's Burst style, a drop within kBurstSeconds of the last one on that
// monster adds to its number instead), placed where the hit landed
// (the monster's + 0x1338, a place the game writes at each hit) or, when that place didn't change (poison, a hit the
// game didn't place there), at the middle of the monster's body (+ 0x168, the centre of its model's bounding sphere,
// whose radius follows at + 0x174), a little above it (NUMBER_LIFT). The app draws the numbers (Numbers()) over the top
// screen's picture, never in it; here each one's place is worked out again every frame with the camera the top screen is
// drawn with: its view from the camera's eye, up and target (camera + 0x40, + 0x50, + 0x60, as the game's 0x00b70fe0
// makes it) and its projection as getProjMatrix gave it this frame (Projection, from hooks.cpp, after the port's
// widescreen and field of view), so the number stays where the hit was while the camera moves. Any drop counts: a
// hunter's hit, a bomb, poison, another player's (the room keeps everyone's health the same).
//
// Monster health bars (monster_health_bar). Once a frame, every large monster of the quest that is alive (not dead, not
// captured), with its health and maximum (+ 0x3e8, + 0x3ec) and whether it is in the hunter's area, in sEnemy's order:
// Healths() for the app, which draws a bar for each over the top screen. Nothing is written, so it is the same in a
// room's quest (where the room keeps every console's health the same) as alone.
#include "hunt_aids.h"

#include "game_addr.h"
#include "hooks.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include <fmt/format.h>

extern "C" {
#include "recomp.h"
}

namespace Port::HuntAids {

namespace {

// (game addresses: GameAddr, port/src/game_addr.h and config/port_addresses.txt; USA v1.3.0's values)
GameAddr SENEMY{0x00f32004};          // sEnemy*: 32 monster slots at +0x18 (0x0068fb9c)
GameAddr MONSTER_TABLE{0x00f3a020};   // [[[[table] + 4*kind]]]: bit 0 a large monster (0x0068882c)
GameAddr GAME_CONTROL{0x00fb6b7c};    // sGameControl*
GameAddr SCAMERA_INSTANCE{0x0105729c};  // sMhCamera*
GameAddr FRAME_COUNTER{0x010571d4};   // u32: the game's frame counter (hunt_trace's lines: the runner's fN)
GameAddr SPLAYER{0x00fb83e8};         // sPlayer*: the hunters' units at +0xa98, by player number

constexpr u32 ENEMY_SLOTS = 0x18, ENEMY_SLOT_COUNT = 32;
constexpr u32 EM_INFO = 0xe28;           // monster: its status block
constexpr u32 INFO_AREA = 0x14c;         // u8: the area of the map it is in
constexpr u32 INFO_KIND = 0x1c0;         // u8: its row of the monster table
constexpr u32 INFO_STATE = 0x1ca;        // u8: 0xb, 0xe dead; 0xc with +0x1cb 0xff captured (0x0094d770)
constexpr u32 INFO_PAINT = 0x20c;        // f32: the paint's time left, painted while above 0 (0x0095541c)
constexpr u32 INFO_HEALTH = 0x3e8;       // s32: health; + 4 its maximum (0x0091ad08)
constexpr u32 UNIT_CENTER = 0x168;       // vec3: the centre of the model's bounding sphere (+ 0x174 its radius)
constexpr u32 UNIT_HIT = 0x1338;         // vec3: where the last hit landed, written as it lands
constexpr u32 GAME_CONTROL_MODE = 0x2b;  // u8: 1 a quest
constexpr u32 GAME_CONTROL_AREA = 0xc90; // u32: the hunter's area of the map
constexpr u32 GAME_CONTROL_LANGUAGE = 0x11;  // u8: the game's language, its folders' row (0x002ffeb4, 0x005aec4c)
constexpr u32 SPLAYER_UNITS = 0xa98;     // uPlayer*[4], by player number (0 for a place nobody plays)
constexpr u32 SCAMERA_VIEWPORTS = 0x18, VIEWPORT_SIZE = 0x1a8, VIEWPORT_CAMERA = 0x04, VIEWPORT_DISPLAY = 0x16;
constexpr u32 CAMERA_EYE = 0x40, CAMERA_UP = 0x50, CAMERA_TARGET = 0x60;  // vec3s (0x00b70fe0)

constexpr float PAINT_KEEP = 7400.0f;    // the timer kept on a monster: above the Palico's 7200 (0x00985edc)
constexpr float PAINT_LOW = 7300.0f;     // under this it is set to PAINT_KEEP again
constexpr float NUMBER_LIFT = 20.0f;     // a number starts this far above the place (a hunter's eye is 170 up)
constexpr int MAX_NUMBERS = 24;

// ── MH3U (program 00040000000AE400, USA v1.0; docs/MH3U.md, docs/research/hunt-aids-mh3u.md): the same aids on its
// own objects, whose addresses (hunt_aids_mh3u.inc, included by hunt_aids.h) are read only while SetMh3u(true).

enum class Game { Mh4u, Mh3u };
Game running_game = Game::Mh4u;
bool Mh3u() {
    return running_game == Game::Mh3u;
}

// The status block's and the unit's fields the aids read, MH4U's or MH3U's (SetGame)
struct Fields {
    u32 info_area = INFO_AREA, info_kind = INFO_KIND, info_paint = INFO_PAINT, info_health = INFO_HEALTH;
    u32 unit_center = UNIT_CENTER, unit_hit = UNIT_HIT;
    float paint_keep = PAINT_KEEP, paint_low = PAINT_LOW;
};
Fields fields;

struct Options {
    bool monsters_on_map = false;
    bool damage_numbers = false;
    bool burst = false;       // (damage_number_burst, 1.0.2: the app's Burst style)
    bool health_bar = false;  // (monster_health_bar: Healths() for the app's bars)
    bool trace = false;
    bool cost = false;  // (tests: MH4N_HUNT_COST=1, CostMeter's line without hunt_trace's others)
};
Options options;
void (*log_line)(const std::string&) = nullptr;

void Log(const std::string& line) {
    if (log_line != nullptr) log_line(line);
}

bool IsOn(const std::string& v) {
    return v == "1" || v == "true" || v == "on" || v == "yes";
}

float ReadF32(RCtx* ctx, u32 addr) {
    const u32 v = rd32(ctx, addr);
    float f;
    std::memcpy(&f, &v, sizeof(f));
    return f;
}

void WriteF32(RCtx* ctx, u32 addr, float f) {
    u32 v;
    std::memcpy(&v, &f, sizeof(v));
    wr32(ctx, addr, v);
}

bool HeapPointer(u32 p) {
    // (MH3U keeps many of its objects, its areas and units among them, in the linear heap too)
    return (p & 3) == 0 && ((p >= 0x08000000 && p < 0x10000000) || (Mh3u() && p >= 0x14000000 && p < 0x1c000000));
}

/// The code's data, the application heap or the linear heap (the monster table's rows live there).
bool DataPointer(u32 p) {
    return (p & 3) == 0 && ((p >= 0x00100000 && p < 0x01200000) || (p >= 0x08000000 && p < 0x10000000) ||
                            (p >= 0x14000000 && p < 0x1c000000) || (p >= 0x30000000 && p < 0x38000000));
}

/// MH3U: `monster`'s status block (0 for an empty slot), and whether it is a large monster.
u32 Mh3uInfo(RCtx* ctx, u32 monster, bool* large);

/// `monster`'s status block if it is a large monster (the game's test, 0x0068882c), else 0.
u32 LargeMonsterInfo(RCtx* ctx, u32 monster) {
    if (Mh3u()) {
        bool large = false;
        const u32 info = Mh3uInfo(ctx, monster, &large);
        return large ? info : 0;
    }
    const u32 info = HeapPointer(monster) ? rd32(ctx, monster + EM_INFO) : 0;
    const u32 table = HeapPointer(info) ? rd32(ctx, MONSTER_TABLE) : 0;
    const u32 entry = DataPointer(table) ? rd32(ctx, table + 4 * rd8(ctx, info + INFO_KIND)) : 0;
    const u32 row = DataPointer(entry) ? rd32(ctx, entry) : 0;
    return DataPointer(row) && (rd32(ctx, row) & 1) != 0 ? info : 0;
}

bool Mh3uDead(RCtx* ctx, u32 info);

bool Dead(RCtx* ctx, u32 info) {
    if (Mh3u()) return Mh3uDead(ctx, info);
    const u8 state = rd8(ctx, info + INFO_STATE);
    return state == 0xb || state == 0xe || (state == 0xc && rd8(ctx, info + INFO_STATE + 1) == 0xff);
}

std::int64_t SteadyNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// ── The camera of the top screen: its projection as getProjMatrix gave it (Projection), its view from its fields

struct CameraSeen {
    u32 camera = 0;
    u64 frame = 0;     // the frame it was seen in (frames below)
    float proj[16] = {};
};
CameraSeen cameras[8];
u64 frames = 0;        // Frame()s since the boot

/// The view matrix the game's 0x00b70ff0 makes from an eye, a target and an up vector (row vectors: p' = p * view):
/// f the eye's direction, s = up x f, u = f x s, then rows (-s.x, u.x, -f.x, 0) ... (s.e, -u.e, f.e, 1).
void LookAt(const float e[3], const float t[3], const float up[3], float view[16]) {
    float f[3] = {t[0] - e[0], t[1] - e[1], t[2] - e[2]};
    const float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (fl > 1e-6f) {
        for (float& c : f) c /= fl;
    }
    float s[3] = {up[1] * f[2] - up[2] * f[1], up[2] * f[0] - up[0] * f[2], up[0] * f[1] - up[1] * f[0]};
    const float sl = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    if (sl > 1e-6f) {
        for (float& c : s) c /= sl;
    }
    const float u[3] = {f[1] * s[2] - f[2] * s[1], f[2] * s[0] - f[0] * s[2], f[0] * s[1] - f[1] * s[0]};
    for (int r = 0; r < 3; ++r) {
        view[r * 4 + 0] = -s[r];
        view[r * 4 + 1] = u[r];
        view[r * 4 + 2] = -f[r];
        view[r * 4 + 3] = 0.0f;
    }
    view[12] = s[0] * e[0] + s[1] * e[1] + s[2] * e[2];
    view[13] = -(u[0] * e[0] + u[1] * e[1] + u[2] * e[2]);
    view[14] = f[0] * e[0] + f[1] * e[1] + f[2] * e[2];
    view[15] = 1.0f;
}

/// p * m for a point (w = 1) and a row-vector matrix.
void Transform(const float m[16], const float p[3], float out[4]) {
    for (int c = 0; c < 4; ++c) {
        out[c] = p[0] * m[c] + p[1] * m[4 + c] + p[2] * m[8 + c] + m[12 + c];
    }
}

struct Camera {
    bool known = false;
    float view[16] = {}, proj[16] = {};
};

/// The camera the top screen's 3D picture is drawn with this frame: the first top-screen viewport's whose projection
/// getProjMatrix gave this frame or the last.
Camera TopCamera(RCtx* ctx) {
    Camera c;
    const bool m3 = Mh3u();
    const u32 scamera = rd32(ctx, m3 ? mh3u::SCAMERA : static_cast<u32>(SCAMERA_INSTANCE));
    if (!HeapPointer(scamera)) {
        return c;
    }
    const u32 viewports = m3 ? mh3u::SCAMERA_VIEWPORTS : SCAMERA_VIEWPORTS;
    const u32 viewport_size = m3 ? mh3u::VIEWPORT_SIZE : VIEWPORT_SIZE;
    const u32 display = m3 ? mh3u::VIEWPORT_DISPLAY : VIEWPORT_DISPLAY;
    for (u32 i = 0; i < 8 && !c.known; ++i) {
        const u32 vp = scamera + viewports + i * viewport_size;
        const u32 camera = rd32(ctx, vp + VIEWPORT_CAMERA);
        if (!HeapPointer(camera) || rd8(ctx, vp + display) != 0) {
            continue;
        }
        // (MH3U: without a getProjMatrix hook of its own the projection the camera keeps, the console's shape)
        CameraSeen kept;
        const CameraSeen* found = nullptr;
        for (const CameraSeen& seen : cameras) {
            if (seen.camera == camera && frames - seen.frame <= 1 && seen.proj[15] == 0.0f) found = &seen;
        }
        if (found == nullptr && m3) {
            kept.camera = camera, kept.frame = frames;
            for (int k = 0; k < 16; ++k) kept.proj[k] = ReadF32(ctx, camera + mh3u::CAMERA_PROJ + 4 * k);
            if (kept.proj[15] == 0.0f && kept.proj[0] != 0.0f) found = &kept;
        }
        if (found == nullptr) {
            continue;
        }
        float e[3], up[3], t[3];
        for (int k = 0; k < 3; ++k) {
            e[k] = ReadF32(ctx, camera + CAMERA_EYE + 4 * k);
            up[k] = ReadF32(ctx, camera + CAMERA_UP + 4 * k);
            t[k] = ReadF32(ctx, camera + CAMERA_TARGET + 4 * k);
        }
        LookAt(e, t, up, c.view);
        std::memcpy(c.proj, found->proj, sizeof(c.proj));
        c.known = std::all_of(std::begin(c.view), std::end(c.view), [](float v) { return std::isfinite(v); });
    }
    return c;
}

/// Where the world point p is on the top screen's picture (fractions from its left and top edges); false behind the
/// camera or off the picture.
bool Project(const Camera& c, const float p[3], float* fx, float* fy) {
    float v[4], clip[4];
    Transform(c.view, p, v);
    Transform(c.proj, v, clip);
    if (!(clip[3] > 1e-3f)) {
        return false;
    }
    const float nx = clip[0] / clip[3], ny = clip[1] / clip[3];
    *fx = (nx + 1.0f) * 0.5f;
    *fy = (1.0f - ny) * 0.5f;
    return std::isfinite(*fx) && std::isfinite(*fy) && *fx > -0.1f && *fx < 1.1f && *fy > -0.1f && *fy < 1.1f;
}

// ── The monsters seen this quest

struct Seen {
    u32 monster = 0, info = 0;
    u8 kind = 0;
    s32 health = 0;
    bool painted_by_us = false;
    u64 frame = 0;    // last seen
    float hit[3] = {};  // the monster's last hit place (fields.unit_hit) as it was at the last number
};
std::vector<Seen> seen;
bool in_quest = false;

Seen* Find(u32 monster) {
    for (Seen& s : seen) {
        if (s.monster == monster) return &s;
    }
    return nullptr;
}

/// A timer of ours back to 0 (the option off, the quest over): only one still in our band (not one the player's
/// paintball set since).
void Unpaint(RCtx* ctx, Seen& s) {
    if (s.painted_by_us && HeapPointer(s.info) && rd32(ctx, s.monster + EM_INFO) == s.info &&
        ReadF32(ctx, s.info + fields.info_paint) <= fields.paint_keep) {
        WriteF32(ctx, s.info + fields.info_paint, 0.0f);
    }
    s.painted_by_us = false;
}

// ── The numbers (the world place each one was dealt at; Numbers() gives them placed on the screen)

struct Dealt {
    float world[3];
    Number number;
    u32 monster = 0;  // (Burst: the run's monster)
};
std::vector<Dealt> dealt;
u32 next_id = 1;

// (MH3U, "By hit") the hits on monsters this game's damage step worked out (Mh3uPartDamage), each waiting for its
// monster's health to drop (up to HIT_WAIT frames): the drop is then shared among them, each its own number with what it
// was. A hit whose drop never comes (a room's guest: the host's game works the health out) shows its own total, which
// the monster's next drops pay back first (credit), so that no damage is shown twice.
struct Hit {
    u32 monster = 0;
    float world[3] = {};
    bool placed = false;  // (world is where the hit landed; else the body's middle when it is shown)
    int total = 0, elem = 0;
    u8 kind = kHitNormal, element = 0;
    s8 crit = 0;
    int frames = 0;
};
constexpr int HIT_WAIT = 12;
constexpr std::int64_t CREDIT_NS = 3000000000;
constexpr size_t MAX_HITS = 32;
std::vector<Hit> hits;
struct Credit {
    u32 monster = 0;
    int left = 0;
    std::int64_t since = 0;
};
std::vector<Credit> credits;
u32 hunter_area = 0;  // (the hunter's area of the map, as the last frame read it)
std::mutex numbers_mutex;
std::vector<Number> published;  // Numbers(), under the mutex
std::atomic<bool> any_published{false};

void Publish(std::vector<Number>&& numbers) {
    std::lock_guard lock(numbers_mutex);
    published = std::move(numbers);
    any_published.store(!published.empty(), std::memory_order_relaxed);
}

// ── The health bars (monster_health_bar): only read, so the same in a room's quest as alone
std::mutex healths_mutex;
std::vector<Health> healths;  // Healths(), under the mutex
std::atomic<bool> any_healths{false};
std::atomic<int> language{-1};  // GameLanguage()

void PublishHealths(std::vector<Health>&& list) {
    if (list.empty() && !any_healths.load(std::memory_order_relaxed)) return;
    std::lock_guard lock(healths_mutex);
    healths = std::move(list);
    any_healths.store(!healths.empty(), std::memory_order_relaxed);
}

}  // namespace

std::vector<Number> Numbers() {
    if (!any_published.load(std::memory_order_relaxed)) {
        return {};
    }
    std::lock_guard lock(numbers_mutex);
    const std::int64_t now = SteadyNs();
    std::vector<Number> out;
    for (const Number& n : published) {  // (a Burst run lasts from its last hit)
        if (now - n.last_ns <= static_cast<std::int64_t>(NumberSeconds() * 1e9)) out.push_back(n);
    }
    return out;
}

std::vector<Health> Healths() {
    if (!any_healths.load(std::memory_order_relaxed)) {
        return {};
    }
    std::lock_guard lock(healths_mutex);
    return healths;
}

bool SetOption(const std::string& key, const std::string& value) {
    if (key == "monster_health_bar") {
        options.health_bar = IsOn(value);
        if (!options.health_bar) PublishHealths({});
    } else if (key == "monsters_on_map") {
        options.monsters_on_map = IsOn(value);
    } else if (key == "damage_numbers") {
        options.damage_numbers = IsOn(value);
        if (!options.damage_numbers) {
            dealt.clear();
            hits.clear(), credits.clear();
            Publish({});
        }
    } else if (key == "damage_number_burst") {
        options.burst = IsOn(value);
    } else if (key == "hunt_trace") {
        options.trace = IsOn(value);
    } else {
        return false;
    }
    return true;
}

bool MonstersOnMap() {
    return options.monsters_on_map;
}

bool DamageNumbersOn() {
    return options.damage_numbers;
}

bool HealthBarOn() {
    return options.health_bar;
}

double NumberSeconds() {
    static const double seconds = [] {
        const char* v = std::getenv("MH4N_TEST_NUMBER_SECONDS");
        const double t = v ? std::atof(v) : 0.0;
        return t >= 1.0 && t <= 30.0 ? t : kNumberSeconds;
    }();
    return seconds;
}

int GameLanguage() {
    return language.load(std::memory_order_relaxed);
}

void SetMh3u(bool on) {
    running_game = on ? Game::Mh3u : Game::Mh4u;
    fields = Fields{};
    if (on) {
        fields.info_area = mh3u::ST_AREA, fields.info_kind = mh3u::ST_KIND, fields.info_paint = mh3u::ST_PAINT;
        fields.info_health = mh3u::ST_HEALTH, fields.unit_center = mh3u::UNIT_CENTER, fields.unit_hit = mh3u::UNIT_HIT;
        fields.paint_keep = mh3u::PAINT_KEEP, fields.paint_low = mh3u::PAINT_LOW;
    }
}

bool IsMh3u() {
    return Mh3u();
}

void Reset() {
    SetMh3u(false);
    options = Options{};
    language.store(-1, std::memory_order_relaxed);
    if (const char* v = std::getenv("MH4U_MONSTERS_ON_MAP")) options.monsters_on_map = IsOn(v);
    if (const char* v = std::getenv("MH4U_DAMAGE_NUMBERS")) options.damage_numbers = IsOn(v);
    if (const char* v = std::getenv("MH4N_HUNT_TRACE")) options.trace = IsOn(v);
    if (const char* v = std::getenv("MH4N_HUNT_COST")) options.cost = IsOn(v);
    seen.clear();
    dealt.clear();
    hits.clear(), credits.clear();
    in_quest = false;
    frames = 0;
    for (CameraSeen& c : cameras) c = CameraSeen{};
    Publish({});
    PublishHealths({});
}

void SetLog(void (*log)(const std::string& line)) {
    log_line = log;
}

void Projection(RCtx* ctx, u32 camera, u32 matrix) {
    if (!options.damage_numbers && !options.trace) {
        return;
    }
    CameraSeen* slot = nullptr;
    for (CameraSeen& c : cameras) {
        if (c.camera == camera) {
            slot = &c;
            break;
        }
    }
    if (slot == nullptr) {  // (the oldest one's place)
        slot = &*std::min_element(std::begin(cameras), std::end(cameras),
                                  [](const CameraSeen& a, const CameraSeen& b) { return a.frame < b.frame; });
    }
    slot->camera = camera;
    slot->frame = frames;
    for (int k = 0; k < 16; ++k) slot->proj[k] = ReadF32(ctx, matrix + 4 * k);
}

/// hunt_trace (or MH4N_HUNT_COST=1): what Frame costs, a line every 3600 frames (the average and the longest, in
/// microseconds).
struct CostMeter {
    std::int64_t start = options.trace || options.cost ? SteadyNs() : 0;
    ~CostMeter() {
        if (start == 0) return;
        static std::int64_t total = 0, longest = 0;
        static u32 count = 0;
        const std::int64_t ns = SteadyNs() - start;
        total += ns, longest = std::max(longest, ns);
        if (++count == 3600) {
            Log(fmt::format("hunt aids: a frame's step took {:.2f} us on average, {:.2f} us at most, over {} frames",
                            total / 1e3 / count, longest / 1e3, count));
            total = longest = 0, count = 0;
        }
    }
};

namespace {

// ── MH3U's monsters (docs/research/hunt-aids-mh3u.md)

u32 Mh3uInfo(RCtx* ctx, u32 monster, bool* large) {
    *large = false;
    if (!HeapPointer(monster)) return 0;
    const u32 state = rd32(ctx, monster + 4) & 7;  // (sEnemy's own test of a slot, 0x005bd89c)
    if (state != 1 && state != 2) return 0;
    const u32 info = rd32(ctx, monster + mh3u::EM_STATUS);
    if (!HeapPointer(info)) return 0;
    *large = (rd32(ctx, info + mh3u::ST_FLAGS) & 1) != 0;  // (the game's own test: the Target Cam's list, 0x0039d278)
    // (tests) MH4N_TEST_MH3U_ANY_LARGE=1: every monster counts as a large one (the bars, the map's paint), to see the
    // aids on the small monsters a new save meets in the Moga Woods
    static const bool any_large = std::getenv("MH4N_TEST_MH3U_ANY_LARGE") != nullptr;
    if (any_large) *large = true;
    return info;
}

bool Mh3uDead(RCtx* ctx, u32 info) {
    // (the monster's end: its damage step sets the health to 0 as it dies or is captured, 0x00838260 .. 0x008382c4,
    // with its end at +0x8ac: 6, 0xf, 0x14 captured, 0x15)
    return static_cast<s32>(rd32(ctx, info + mh3u::ST_HEALTH)) <= 0;
}

int Mh3uHunters(RCtx* ctx) {
    // sPlayer's units: 8 slots from +0x20, a slot good while [unit+4]&7 is 1 or 2 (sPlayer's own getter, 0x008959c0)
    const u32 players = rd32(ctx, mh3u::SPLAYER);
    int n = 0;
    for (u32 i = 0; i < 8 && HeapPointer(players); ++i) {
        const u32 unit = rd32(ctx, players + mh3u::SPLAYER_UNITS + 4 * i);
        const u32 state = HeapPointer(unit) ? rd32(ctx, unit + 4) & 7 : 0;
        n += state == 1 || state == 2 ? 1 : 0;
    }
    return n;
}

u32 FrameCount(RCtx* ctx);

/// One number of `value` (and `elem` of the element, By hit's second number) at the world place `at` (lifted), as a new
/// number or (Burst) added to this monster's run still going; not for a monster in another area of the map.
void AddNumber(RCtx* ctx, u32 monster, bool here, const float at[3], int value, int elem, u8 kind, s8 crit, u8 element,
               std::int64_t now, const char* where) {
    static const char* const kKinds[] = {"plain", "normal", "a weak point", "hard", "fixed"};
    Dealt d{};
    std::memcpy(d.world, at, sizeof d.world);
    d.world[1] += NUMBER_LIFT;
    d.number.value = value;
    d.number.elem = elem;
    d.number.element = element;
    d.number.kind = kind;
    d.number.crit = crit;
    d.number.born_ns = d.number.last_ns = now;
    d.number.id = next_id++;
    d.monster = monster;
    Dealt* run = nullptr;  // (Burst: this monster's number of a run still going)
    for (Dealt& o : dealt) {
        if (options.burst && o.monster == monster &&
            now - o.number.last_ns < static_cast<std::int64_t>(kBurstSeconds * 1e9))
            run = &o;
    }
    if (run != nullptr) {
        run->number.value += value, run->number.elem += elem, run->number.hits++, run->number.last_ns = now;
        if (kind == kHitWeak || run->number.kind == kHitPlain) run->number.kind = kind;  // (a run: its best hit's look)
        if (crit > 0) run->number.crit = crit;
        if (element) run->number.element = element;
    } else if (here && std::isfinite(d.world[0] + d.world[1] + d.world[2])) {
        if (dealt.size() >= MAX_NUMBERS) dealt.erase(dealt.begin());
        dealt.push_back(d);
    }
    if (options.trace) {
        Log(fmt::format("hunt aids: f{} monster {:08X}: a number of {}{} at the {}, {}{}{}", FrameCount(ctx), monster,
                        value, elem ? fmt::format(" + {} of element {}", elem, element) : std::string(), where,
                        kKinds[std::min<int>(kind, 4)], crit > 0 ? ", critical" : crit < 0 ? ", feeble" : "",
                        here ? "" : " (in another area: not shown)"));
    }
}

/// The hits whose drop didn't come in HIT_WAIT frames (a room's guest): their own totals, owed to the next drops.
void AgeHits(RCtx* ctx, std::int64_t now) {
    for (auto it = hits.begin(); it != hits.end();) {
        if (++it->frames <= HIT_WAIT) {
            ++it;
            continue;
        }
        const Seen* s = Find(it->monster);
        float at[3];
        if (it->placed || s == nullptr) {
            std::memcpy(at, it->world, sizeof at);
        } else {
            for (int k = 0; k < 3; ++k) at[k] = ReadF32(ctx, it->monster + fields.unit_center + 4 * k);
        }
        const bool here = s != nullptr && HeapPointer(s->info) && rd8(ctx, s->info + fields.info_area) == hunter_area;
        AddNumber(ctx, it->monster, here, at, it->total - it->elem, it->elem, it->kind, it->crit, it->element, now,
                  it->placed ? "hit (no drop of its health)" : "body's middle (no drop of its health)");
        Credit* c = nullptr;
        for (Credit& o : credits) {
            if (o.monster == it->monster) c = &o;
        }
        if (c == nullptr) credits.push_back(Credit{it->monster, 0, now}), c = &credits.back();
        if (now - c->since > CREDIT_NS) c->left = 0;
        c->left += it->total, c->since = now;
        it = hits.erase(it);
    }
}

u32 FrameCount(RCtx* ctx) {
    return rd32(ctx, Mh3u() ? mh3u::FRAME_COUNTER : static_cast<u32>(FRAME_COUNTER));
}

/// Where the hunter is this frame: in a quest (with the monsters' table), the area of the map, how many hunters the
/// quest has, and the game's language (MH4U's numbering: 1 eng, 2 fre, 3 spa, 4 ger, 5 ita; -1 unknown).
struct Place {
    bool quest = false;
    u32 enemies = 0, area = 0;
    int hunters = 0;
    int language = -1;
};

/// MH3U's language row (its suffix table 0x00caa064, 0x0053afb4: jpn, eng, fre, spa, ger, ita, kor, chT, chS, por,
/// rus ...) in MH4U's numbering, which the app's names follow: the same rows (seen in a dump: an English console's 1)
int Mh3uLanguage(int row) {
    return row >= 0 && row < 21 ? row : -1;
}

/// MH3U: the class (its MtDTI) of an object of the game's, from its getDTI (vtable slot 5: "ldr r0, [pc, #n]; bx lr");
/// 0 when it isn't one.
u32 Mh3uClass(RCtx* ctx, u32 object) {
    const u32 vtable = rd32(ctx, object);
    if (!DataPointer(vtable)) return 0;
    const u32 getdti = rd32(ctx, vtable + 0x14);
    if (getdti < 0x00100000 || getdti >= mh3u::CODE_END || (getdti & 3) != 0) return 0;
    const u32 insn = rd32(ctx, getdti);
    if ((insn & 0xfffff000) != 0xe59f0000 || rd32(ctx, getdti + 4) != 0xe12fff1e) return 0;
    return rd32(ctx, getdti + 8 + (insn & 0xfff));
}

Place ReadPlace(RCtx* ctx) {
    Place p;
    if (Mh3u()) {
        const u32 gui = rd32(ctx, mh3u::SGUI);
        if (HeapPointer(gui) || DataPointer(gui)) p.language = Mh3uLanguage(static_cast<s8>(rd8(ctx, gui + mh3u::GUI_LANGUAGE)));
        const u32 areas = rd32(ctx, mh3u::SAREA);
        if (HeapPointer(areas)) {  // (a quest: one of the areas running is aQuest)
            const u32 count = std::min<u32>(rd32(ctx, areas + mh3u::SAREA_COUNT), 8);
            for (u32 i = 0; i < count && !p.quest; ++i) {
                const u32 a = rd32(ctx, areas + mh3u::SAREA_TABLE + 4 * i);
                const u32 dti = HeapPointer(a) ? Mh3uClass(ctx, a) : 0;
                p.quest = dti == mh3u::DTI_AQUEST || dti == mh3u::DTI_AQUEST_AREA || dti == mh3u::DTI_AFREEQUEST ||
                          dti == mh3u::DTI_AFREEQUEST_AREA;
            }
        }
        if (!p.quest) return p;
        p.enemies = rd32(ctx, mh3u::SENEMY);
        const u32 stage = rd32(ctx, mh3u::SSTAGE);
        p.area = HeapPointer(stage) ? rd8(ctx, stage + mh3u::STAGE_AREA) : 0;
        p.hunters = Mh3uHunters(ctx);
        return p;
    }
    const u32 control = rd32(ctx, GAME_CONTROL);
    if (!HeapPointer(control)) return p;
    p.language = rd8(ctx, control + GAME_CONTROL_LANGUAGE);
    p.quest = rd8(ctx, control + GAME_CONTROL_MODE) == 1;
    if (!p.quest) return p;
    p.enemies = rd32(ctx, SENEMY);
    p.area = rd32(ctx, control + GAME_CONTROL_AREA) & 0xff;
    const u32 players = rd32(ctx, SPLAYER);
    for (u32 n = 0; n < 4 && HeapPointer(players); ++n) {
        p.hunters += HeapPointer(rd32(ctx, players + SPLAYER_UNITS + 4 * n)) ? 1 : 0;
    }
    return p;
}

}  // namespace

bool HitKindsKnown() {
    return Mh3u();
}

void Mh3uPartDamage(RCtx* ctx, void (*original)(RCtx*)) {
    const u32 monster = ctx->r[0], lr = ctx->r[14], hit = ctx->r[4];
    const int total = static_cast<s32>(ctx->r[2]);
    if (options.damage_numbers && Mh3u() && lr == mh3u::HIT_STEP_RETURN && HeapPointer(monster) && HeapPointer(hit) &&
        total > 0 && hits.size() < MAX_HITS) {
        float raw = ctx->f.s[16], elem = ctx->f.s[17];
        if (!(raw >= 0 && elem >= 0 && raw + elem > 0) || !std::isfinite(raw + elem)) raw = static_cast<float>(total), elem = 0;
        Hit h;
        h.monster = monster;
        h.total = total;
        h.elem = static_cast<int>(std::lround(total * elem / (raw + elem)));
        const u32 elements = rd32(ctx, hit + mh3u::HIT_ELEMENTS);
        h.element = h.elem <= 0 ? 0 : elements & 0x10 ? 1 : elements & 0x20 ? 2 : elements & 0x200 ? 3 : elements & 0x40 ? 4
                                   : elements & 0x80 ? 5 : 0;
        if (h.element == 0) h.elem = 0;
        const u8 level = rd8(ctx, hit + mh3u::HIT_LEVEL);
        const u32 marks = rd16(ctx, hit + mh3u::HIT_MARKS);  // (the attacker's affinity roll: 0x200 critical, 0x100 feeble)
        h.crit = marks & 0x200 ? 1 : marks & 0x100 ? -1 : 0;
        h.kind = rd8(ctx, hit + mh3u::HIT_FIXED) == 0x1f ? kHitFixed
                 : level == 3 || level == 4             ? kHitWeak
                 : level == 0                           ? kHitHard
                                                        : kHitNormal;
        // where it landed: the middle of the two points of the contact, which the step's caller (0x009838a4) hands it
        // as its last argument (the step's frame + HIT_STEP_PLACE: its frame is the caller's at this call)
        const u32 place = rd32(ctx, ctx->r[13] + mh3u::HIT_STEP_PLACE);
        if (HeapPointer(place)) {
            for (int k = 0; k < 3; ++k) h.world[k] = ReadF32(ctx, place + 4 * k);
            h.placed = std::isfinite(h.world[0] + h.world[1] + h.world[2]) &&
                       (h.world[0] != 0 || h.world[1] != 0 || h.world[2] != 0);
        }
        if (!h.placed) {
            for (int k = 0; k < 3; ++k) h.world[k] = ReadF32(ctx, monster + fields.unit_center + 4 * k);
        }
        hits.push_back(h);
        if (options.trace) {
            Log(fmt::format("hunt aids: f{} hit on {:08X}: {} (raw {:.1f}, element {:.1f} of {:x}), level {}, marks {:x}, fixed {:x}, "
                            "part {}, at {:.0f},{:.0f},{:.0f}{}", FrameCount(ctx), monster, total, raw, elem, elements,
                            level, marks, rd8(ctx, hit + mh3u::HIT_FIXED), ctx->r[1], h.world[0], h.world[1], h.world[2],
                            h.placed ? "" : " (the body's middle)"));
        }
    }
    original(ctx);
}


/// (MH3U research, hunt_trace) the local hunter's weapon and action as they change: what the bow's aim needs (its
/// draw and shot), seen with a bow in hand. The hunter: sPlayer + 0x20 + 4 x this console's player number (sGame
/// [0x00c3b978] + 0x2b); its state block [unit + 0xd5c]: + 0x02 the weapon's class (10 the bow), + 0x3e the action's
/// mode, + 0x40 the action (mh3u-qol's notes), + 0x118 the facing.
u32 Mh3uLocalUnit(RCtx* ctx);

// ── MH3U: a bow's shot to the monster (bow_aim, on unless off: MH4U's BowAimStep in hooks.cpp, for MH3U). The arrow
// leaves where the hunter faces, so the facing (mh3u::UNIT_FACING) is written toward a monster as the draw's button (X)
// is let go with the bow out after a draw (that frame whatever the stick) and through the BOW_SHOT_FRAMES frames after
// it while the stick is let be (pushed, the hunter goes its own way). The monster: the Target Cam's while it is on, else
// the nearest large one alive in the hunter's area within BOW_VIEW_CONE of the way the hunter faces and BOW_RANGE of it.
// MH3U's bow has no aiming mode of its own to leave alone (R is its coatings' and the run's). Called at the start of
// each game frame (hooks.cpp's Mh3uFrameTiming), before the hunter's step reads the facing.
constexpr int BOW_SHOT_FRAMES = 12;
constexpr int BOW_DRAW_FRAMES = 4;      // X held this long with the bow out: a draw (a tap only draws the weapon)
constexpr int BOW_VIEW_CONE = 0x2000;   // within 45 degrees of the hunter's facing, either way
constexpr float BOW_RANGE = 3500.0f;    // (the Moga Woods' areas are some thousands across)

int AngleTo(float dx, float dz) {  // (0x10000 a turn, as the facing: atan2(dx, dz))
    return static_cast<int>(std::lround(std::atan2(dx, dz) * 32768.0 / 3.14159265358979323846)) & 0xffff;
}

void Mh3uSetFacing(RCtx* ctx, u32 unit, u16 facing) {
    wr16(ctx, unit + mh3u::UNIT_FACING, facing);
    for (const u32 copy : mh3u::UNIT_FACING_COPIES) wr16(ctx, unit + copy, facing);
}

/// The bow's monster for a shot from (hx, hz) with the hunter facing `look`, or 0.
u32 Mh3uBowTarget(RCtx* ctx, u32 state, float hx, float hz, int look, const char** why) {
    const u32 enemies = rd32(ctx, mh3u::SENEMY);
    if (!HeapPointer(enemies)) return 0;
    const u8 area = rd8(ctx, state + mh3u::ST_HUNTER_AREA);
    const bool cam = rd8(ctx, state + mh3u::ST_TARGET_ON) == 1;
    const u32 cam_slot = rd8(ctx, state + mh3u::ST_TARGET);
    u32 nearest = 0;
    float best = BOW_RANGE * BOW_RANGE;
    for (u32 i = 0; i < ENEMY_SLOT_COUNT; ++i) {
        const u32 monster = rd32(ctx, enemies + mh3u::ENEMY_SLOTS + 4 * i);
        bool large = false;
        const u32 info = Mh3uInfo(ctx, monster, &large);
        if (!large || Mh3uDead(ctx, info) || rd8(ctx, info + mh3u::ST_AREA) != area) continue;
        const float dx = ReadF32(ctx, monster + mh3u::UNIT_CENTER) - hx;
        const float dz = ReadF32(ctx, monster + mh3u::UNIT_CENTER + 8) - hz;
        const float d = dx * dx + dz * dz;
        if (!std::isfinite(d) || d < 1.0f || d >= BOW_RANGE * BOW_RANGE) continue;
        if (cam && i == cam_slot) {
            *why = "the Target Cam's";
            return monster;
        }
        const int off = std::abs(static_cast<s16>(static_cast<u16>(AngleTo(dx, dz) - look)));
        if (off <= BOW_VIEW_CONE && d < best) best = d, nearest = monster;
    }
    *why = "the nearest in view";
    return nearest;
}

void Mh3uBowStep(RCtx* ctx, bool on) {
    static int held_frames = 0;    // frames X has been held with the bow out
    static int shot_frames = 0;    // frames of the shot left to face the monster
    static u32 target = 0;
    const u32 unit = on && Mh3u() ? Mh3uLocalUnit(ctx) : 0;
    const u32 state = unit ? rd32(ctx, unit + mh3u::HUNTER_STATE) : 0;
    if (!HeapPointer(state) || rd8(ctx, state + mh3u::ST_WEAPON) != mh3u::WEAPON_BOW ||
        rd8(ctx, state + mh3u::ST_WEAPON_OUT) == 0) {
        held_frames = shot_frames = 0, target = 0;
        return;
    }
    const u32 pad = rd32(ctx, mh3u::SAPPPAD);
    const u32 held = HeapPointer(pad) ? rd32(ctx, pad + mh3u::PAD_HELD) : 0;
    const bool letting_go = !(held & mh3u::PAD_X) && held_frames >= BOW_DRAW_FRAMES;
    held_frames = (held & mh3u::PAD_X) ? held_frames + 1 : 0;
    const float hx = ReadF32(ctx, state + mh3u::ST_PLACE), hz = ReadF32(ctx, state + mh3u::ST_PLACE + 8);
    if (letting_go) {
        const u16 before = rd16(ctx, unit + mh3u::UNIT_FACING);
        const char* why = "";
        target = std::isfinite(hx + hz) ? Mh3uBowTarget(ctx, state, hx, hz, before, &why) : 0;
        shot_frames = target ? BOW_SHOT_FRAMES : 0;
        if (target) {
            const u16 facing = static_cast<u16>(AngleTo(ReadF32(ctx, target + mh3u::UNIT_CENTER) - hx,
                                                        ReadF32(ctx, target + mh3u::UNIT_CENTER + 8) - hz));
            Mh3uSetFacing(ctx, unit, facing);
            if (options.trace) {
                Log(fmt::format("hunt aids: f{} bow aim: the draw let go, turned to monster {:08X}, {} (facing {:04x} -> "
                                "{:04x})", FrameCount(ctx), target, why, before, facing));
            }
        } else if (options.trace) {
            Log(fmt::format("hunt aids: f{} bow aim: the draw let go, no monster in view", FrameCount(ctx)));
        }
        return;
    }
    if (shot_frames <= 0 || !target) return;
    --shot_frames;
    if (held & mh3u::PAD_STICK) return;  // (the stick pushed: the hunter's own way)
    const float mx = ReadF32(ctx, target + mh3u::UNIT_CENTER), mz = ReadF32(ctx, target + mh3u::UNIT_CENTER + 8);
    if (std::isfinite(mx + mz + hx + hz)) Mh3uSetFacing(ctx, unit, static_cast<u16>(AngleTo(mx - hx, mz - hz)));
}

void Mh3uBow(RCtx* ctx, bool on) {
    Mh3uBowStep(ctx, on);
}

u32 Mh3uLocalUnit(RCtx* ctx) {  // (the local hunter's unit, or 0)
    const u32 game = rd32(ctx, mh3u::SGAME), players = rd32(ctx, mh3u::SPLAYER);
    if (!HeapPointer(game) || !HeapPointer(players)) return 0;
    const int n = static_cast<s8>(rd8(ctx, game + mh3u::SGAME_PLAYER));
    if (n < 0 || n >= 8) return 0;
    const u32 unit = rd32(ctx, players + mh3u::SPLAYER_UNITS + 4 * n);
    return HeapPointer(unit) ? unit : 0;
}

void Mh3uActionTrace(RCtx* ctx) {
    static u32 last = 0xffffffff;
    const u32 unit = Mh3uLocalUnit(ctx);
    const u32 state = unit ? rd32(ctx, unit + mh3u::HUNTER_STATE) : 0;
    if (!HeapPointer(state)) return;
    static u32 last_held = 0;
    const u32 now = rd8(ctx, state + 0x02) << 24 | rd8(ctx, state + 0x3e) << 16 | rd16(ctx, state + 0x40);
    const u32 pad = rd32(ctx, mh3u::SAPPPAD);
    const u32 held = HeapPointer(pad) ? rd32(ctx, pad + mh3u::PAD_HELD) : 0;
    if (now != last || held != last_held) {
        Log(fmt::format("hunt aids: f{} hunter weapon {} mode {} action {:04x} held {:08x} facing {:04x} {:04x} {:04x} "
                        "at {:.0f},{:.0f}", FrameCount(ctx), now >> 24, (now >> 16) & 0xff, now & 0xffff, held,
                        rd16(ctx, state + mh3u::STATE_FACING), rd16(ctx, state + mh3u::STATE_FACING2),
                        rd16(ctx, state + mh3u::STATE_FACING3), ReadF32(ctx, state + 0xd8), ReadF32(ctx, state + 0xe0)));
        last = now, last_held = held;
    }
}

// (research, MH3U) MH4N_TEST_MH3U_WEAR=<kind>:<id>: out of a quest, the box's first piece of that kind and id swapped
// with the weapon worn, once (the test save's Hunter's Bow I: 17:1), so a run hunts with it.
// MH4N_TEST_MH3U_FACE_SPIN=<hex offsets of the hunter's unit, comma separated>: in a quest those u16s turned by 1/64 of
// a turn each frame and logged every second (the one that is the facing turns the hunter round where it stands)
void Mh3uTests(RCtx* ctx, bool quest) {
    static int wear = -1, wear_kind = 0, wear_id = 0;
    static std::vector<u32> spin;
    if (wear < 0) {
        wear = 0;
        if (const char* v = std::getenv("MH4N_TEST_MH3U_WEAR"); v != nullptr && std::strchr(v, ':') != nullptr) {
            wear_kind = std::atoi(v), wear_id = std::atoi(std::strchr(v, ':') + 1), wear = 1;
        }
        if (const char* v = std::getenv("MH4N_TEST_MH3U_FACE_SPIN")) {
            for (const char* c = v; *c;) {
                char* end = nullptr;
                const unsigned long off = std::strtoul(c, &end, 16);
                if (end == c) break;
                spin.push_back(static_cast<u32>(off));
                c = *end == ',' ? end + 1 : end;
            }
        }
    }
    static u8 was[16] = {}, now_worn[16] = {};  // (the weapon worn before the swap and after it)
    if (wear == 1 && !quest && frames % 30 == 0) {  // (until the save is loaded: the box is empty before)
        const u32 user = rd32(ctx, mh3u::SUSERDATA);
        if (HeapPointer(user) && rd8(ctx, user + mh3u::USER_BOX) != 0) {
            wear = 2;
            for (u32 i = 0; i < mh3u::BOX_SLOTS; ++i) {
                const u32 slot = user + mh3u::USER_BOX + mh3u::BOX_SLOT * i;
                if (rd8(ctx, slot) != wear_kind || rd16(ctx, slot + 2) != wear_id) continue;
                const u32 worn = user + mh3u::USER_WORN;
                for (u32 k = 0; k < mh3u::BOX_SLOT; ++k) {
                    was[k] = rd8(ctx, worn + k), now_worn[k] = rd8(ctx, slot + k);
                    wr8(ctx, slot + k, was[k]), wr8(ctx, worn + k, now_worn[k]);
                }
                wear = 3;
                Log(fmt::format("hunt aids: test: box slot {} (kind {} id {}) swapped with the weapon worn (kind {} id {})", i,
                                wear_kind, wear_id, was[0], was[2] | was[3] << 8));
                break;
            }
            if (wear != 3) Log("hunt aids: test: that piece is not in the box");
        }
    }
    // (the profile's copies the hunter is made from, sFreeQuest's and sQuest's, made after the swap from another copy:
    // those that hold the old weapon get the new one, out of a quest, every half second)
    if (wear == 3 && !quest && frames % 30 == 0) {
        for (const auto [single, field] : {std::pair{mh3u::SFREEQUEST, mh3u::FREEQUEST_WORN},
                                          std::pair{mh3u::SQUEST, mh3u::QUEST_WORN}}) {
            const u32 o = rd32(ctx, single);
            if (!HeapPointer(o)) continue;
            bool same = true;
            for (u32 k = 0; k < mh3u::BOX_SLOT; ++k) same = same && rd8(ctx, o + field + k) == was[k];
            if (!same) continue;
            for (u32 k = 0; k < mh3u::BOX_SLOT; ++k) wr8(ctx, o + field + k, now_worn[k]);
            Log(fmt::format("hunt aids: test: the copy at [{:08X}] + {:x} too", single, field));
        }
    }
    if (spin.empty() || !quest) return;
    const u32 unit = Mh3uLocalUnit(ctx);
    if (!unit) return;
    static std::int64_t said = 0;
    const std::int64_t now = SteadyNs();
    std::string values;
    for (const u32 f : spin) {
        wr16(ctx, unit + f, static_cast<u16>(rd16(ctx, unit + f) + 0x400));
        values += fmt::format(" +{:x} {:04x}", f, rd16(ctx, unit + f));
    }
    if (now - said > 1'000'000'000) {
        said = now;
        Log(fmt::format("hunt aids: test: face spin:{}", values));
    }
}

void Frame(RCtx* ctx) {
    ++frames;
    if (options.trace && Mh3u()) Mh3uActionTrace(ctx);
    const bool any = options.monsters_on_map || options.damage_numbers || options.health_bar || options.trace;
    if (!any && seen.empty() && !in_quest) {
        return;  // (off: nothing read)
    }
    const CostMeter meter;
    const Place place = ReadPlace(ctx);
    if (Mh3u()) Mh3uTests(ctx, place.quest);
    if (options.health_bar && place.language >= 0) {  // (the bars' names: the app reads them in this language)
        if (language.exchange(place.language, std::memory_order_relaxed) != place.language && options.trace) {
            Log(fmt::format("hunt aids: f{} the game's language {}", FrameCount(ctx), place.language));
        }
    }
    const u32 enemies = place.enemies;
    if (!place.quest || !HeapPointer(enemies)) {
        if (in_quest) {  // (the quest is over: what was ours goes, nothing carries over to the next)
            in_quest = false;
            seen.clear();
            dealt.clear();
            hits.clear(), credits.clear();
            Publish({});
            PublishHealths({});
        }
        return;
    }
    in_quest = true;
    const u32 area = place.area;
    hunter_area = area;
    const int hunters = place.hunters;  // (the hunters in the quest: more than this console's own, a room's quest)
    const bool paint = options.monsters_on_map && hunters <= 1;
    const std::int64_t now = SteadyNs();
    std::vector<Health> bars;
    const u32 slots = Mh3u() ? mh3u::ENEMY_SLOTS : ENEMY_SLOTS;
    for (u32 i = 0; i < ENEMY_SLOT_COUNT; ++i) {
        const u32 monster = rd32(ctx, enemies + slots + 4 * i);
        u32 large_info = 0, info = 0;
        if (Mh3u()) {
            bool large = false;
            info = Mh3uInfo(ctx, monster, &large);
            large_info = large ? info : 0;
            if (!large && !options.damage_numbers) info = 0;
        } else {
            large_info = LargeMonsterInfo(ctx, monster);
            // (1.0.2, T40: the numbers are every monster's, large and small; the paint and the bars a large one's)
            info = large_info != 0          ? large_info
                   : options.damage_numbers && HeapPointer(monster) && HeapPointer(rd32(ctx, monster + EM_INFO))
                       ? rd32(ctx, monster + EM_INFO)
                       : 0;
        }
        if (info == 0) {
            continue;
        }
        const bool large = large_info != 0;
        const u8 kind = rd8(ctx, info + fields.info_kind);
        const s32 health = static_cast<s32>(rd32(ctx, info + fields.info_health));
        if (!large) {  // (a small one: only with a health that reads as one, 0 to its maximum)
            const s32 most = static_cast<s32>(rd32(ctx, info + fields.info_health + 4));
            if (!(most > 0 && most < 100000 && health <= most)) continue;
        }
        Seen* s = Find(monster);
        if (s != nullptr && (s->info != info || s->kind != kind)) {  // (another monster in the same memory)
            *s = Seen{};
            s->monster = monster, s->health = health;
            for (int k = 0; k < 3; ++k) s->hit[k] = fields.unit_hit ? ReadF32(ctx, monster + fields.unit_hit + 4 * k) : 0.0f;
        }
        if (s == nullptr) {
            seen.push_back(Seen{monster, info, kind, health, false, frames});
            s = &seen.back();
            for (int k = 0; k < 3; ++k) s->hit[k] = fields.unit_hit ? ReadF32(ctx, monster + fields.unit_hit + 4 * k) : 0.0f;  // (an older hit's place)
        }
        s->info = info, s->kind = kind, s->frame = frames;
        const bool dead = Dead(ctx, info);
        // the bars: every large monster alive (one in another area too: the app shows it dimmer)
        if (large && options.health_bar && !dead) {
            const s32 max = static_cast<s32>(rd32(ctx, info + fields.info_health + 4));
            if (max > 0 && max < 1000000) {
                bars.push_back(Health{std::clamp<s32>(health, 0, max), max, rd8(ctx, info + fields.info_area) == area, kind,
                                      monster});
            }
        }
        // the map: a paint timer kept on the monster while the option is on
        if (large && paint && !dead) {
            const float paint = ReadF32(ctx, info + fields.info_paint);
            if (!(paint > fields.paint_keep)) {     // (a timer of the player's paintball is left alone)
                if (!(paint >= fields.paint_low)) WriteF32(ctx, info + fields.info_paint, fields.paint_keep);
                s->painted_by_us = true;
            } else {
                s->painted_by_us = false;
            }
        } else if (s->painted_by_us) {
            Unpaint(ctx, *s);
        }
        // the numbers: a drop of health since the last frame (MH3U: shared among the hits that made it)
        if (options.damage_numbers && health < s->health && s->health - health < 100000) {
            float hit[3], center[3];
            for (int k = 0; k < 3; ++k) {
                hit[k] = fields.unit_hit ? ReadF32(ctx, monster + fields.unit_hit + 4 * k) : 0.0f;
                center[k] = ReadF32(ctx, monster + fields.unit_center + 4 * k);
            }
            const bool placed = std::memcmp(hit, s->hit, sizeof hit) != 0 && (hit[0] != 0 || hit[1] != 0 || hit[2] != 0);
            std::memcpy(s->hit, hit, sizeof hit);
            const bool here = rd8(ctx, info + fields.info_area) == area;
            int drop = s->health - std::max(health, 0);
            for (Credit& c : credits) {  // (hits already shown with no drop: paid back first)
                if (c.monster != monster || c.left <= 0) continue;
                if (now - c.since > CREDIT_NS) {
                    c.left = 0;
                    continue;
                }
                const int paid = std::min(c.left, drop);
                c.left -= paid, drop -= paid;
            }
            int sum = 0, n = 0;
            for (const Hit& h : hits) {
                if (h.monster == monster) sum += h.total, ++n;
            }
            if (n > 0 && drop > 0) {
                int left = drop, seen_hits = 0;
                for (auto it = hits.begin(); it != hits.end();) {
                    if (it->monster != monster) {
                        ++it;
                        continue;
                    }
                    const int v = ++seen_hits == n ? left
                                                   : static_cast<int>(static_cast<std::int64_t>(drop) * it->total /
                                                                      std::max(sum, 1));
                    left -= v;
                    const int e =
                        it->total > 0 ? static_cast<int>(static_cast<std::int64_t>(v) * it->elem / it->total) : 0;
                    AddNumber(ctx, monster, here, it->placed ? it->world : center, v - e, e, it->kind, it->crit,
                              it->element, now, it->placed ? "hit" : "body's middle");
                    it = hits.erase(it);
                }
            } else if (drop > 0) {
                AddNumber(ctx, monster, here, placed ? hit : center, drop, 0, kHitPlain, 0, 0, now,
                          placed ? "hit" : "body's middle");
            }
            if (options.trace) {
                Log(fmt::format("hunt aids: f{} monster {:08X} health {} -> {} (max {}): {} of it shown{}{}",
                                FrameCount(ctx), monster, s->health, health,
                                static_cast<s32>(rd32(ctx, info + fields.info_health + 4)), drop,
                                n > 0 ? fmt::format(" for {} hit(s)", n) : std::string(),
                                here ? "" : " (in another area: not shown)"));
            }
        }
        s->health = health;
    }
    // monsters gone (another area's left, a quest's end): forget them; with both options off, all of them (a timer of
    // ours went back to 0 just above: from the next frame on nothing is read)
    if (!hits.empty()) AgeHits(ctx, now);  // (MH3U: the hits whose drop did not come)
    seen.erase(std::remove_if(seen.begin(), seen.end(), [](const Seen& s) { return s.frame != frames; }), seen.end());
    if (!any) {
        seen.clear(), dealt.clear(), in_quest = false;
        Publish({});
        PublishHealths({});
        return;
    }
    if (options.trace && options.health_bar) {  // (every frame: tests draw the bars on the runner's screenshots)
        // (the HUD's parts the app's bars keep clear of: the hunter's own bars, the area map, the target camera)
        const Hooks::GameState game = Hooks::CurrentGameState();
        std::string parts;
        for (const int k : {1, 4, 5}) {
            const Hooks::GameState::HudPart& p = game.hud[k];
            parts += fmt::format(" {} {} {:.3f},{:.3f},{:.3f},{:.3f}", Hooks::HudPartKey(k), p.shown, p.rect[0],
                                 p.rect[1], p.rect[2], p.rect[3]);
        }
        Log(fmt::format("hunt aids: f{} hud menu {}{}", FrameCount(ctx), game.menu, parts));
        for (const Health& h : bars) {
            Log(fmt::format("hunt aids: f{} bar {:08X} kind {:02X} {}/{} {}", FrameCount(ctx), h.id, h.kind,
                            h.health, h.max, h.here ? "here" : "away"));
        }
    }
    PublishHealths(std::move(bars));
    // the numbers on the screen, placed with this frame's camera
    if (options.damage_numbers || !dealt.empty()) {
        dealt.erase(std::remove_if(dealt.begin(), dealt.end(),
                                   [now](const Dealt& d) {
                                       return now - d.number.last_ns > static_cast<std::int64_t>(NumberSeconds() * 1e9);
                                   }),
                    dealt.end());
        const Camera camera = TopCamera(ctx);
        std::vector<Number> out;
        out.reserve(dealt.size());
        for (Dealt& d : dealt) {
            d.number.shown = camera.known && Project(camera, d.world, &d.number.x, &d.number.y);
            out.push_back(d.number);
            if (options.trace) {  // (every frame it lasts: tests draw it on the runner's screenshots, tools of out/)
                Log(fmt::format("hunt aids: f{} number {} {} at {:.4f},{:.4f} age {:.3f}{}", FrameCount(ctx),
                                d.number.id, d.number.value, d.number.x, d.number.y, (now - d.number.born_ns) / 1e9,
                                d.number.shown ? "" : " (not on the picture)"));
            }
        }
        Publish(std::move(out));
    }
    if (options.trace && frames % 60 == 0) {
        const Camera camera = TopCamera(ctx);
        for (const Seen& s : seen) {
            const u32 info = s.info;
            const float p[3] = {ReadF32(ctx, s.monster + fields.unit_center), ReadF32(ctx, s.monster + fields.unit_center + 4),
                                ReadF32(ctx, s.monster + fields.unit_center + 8)};
            float fx = -1, fy = -1;
            const bool on = camera.known && Project(camera, p, &fx, &fy);
            Log(fmt::format("hunt aids: f{} monster {:08X} kind {:02X} area {} (hunter's {}, hunters {}) health {}/{} "
                            "paint {:.0f} "
                            "at {:.0f},{:.0f},{:.0f} screen {}{:.3f},{:.3f} camera {}",
                            FrameCount(ctx), s.monster, s.kind, rd8(ctx, info + fields.info_area), area, hunters,
                            s.health,
                            static_cast<s32>(rd32(ctx, info + fields.info_health + 4)), ReadF32(ctx, info + fields.info_paint),
                            p[0], p[1], p[2], on ? "" : "off ", fx, fy, camera.known ? "known" : "not known"));
        }
    }
}

}  // namespace Port::HuntAids
