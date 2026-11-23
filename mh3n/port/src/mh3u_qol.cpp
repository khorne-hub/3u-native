// MH3U's own versions of the port's gameplay features (mh3u_qol.h). MH3U USA v1.0's addresses, found in its code
// (each one checked in the game's code and in play). Nothing here runs on an MH4U library.
//
// The camera (uGameCamera, its instance at sGameCamera [0x00c3b9cc] + 0x14; its data at [camera + 0x8e8]). Each frame the
// hunter's camera step (0x0026766c) sets the data's yaw (+0x94, a u16 angle in a u32: 0x10000 a turn, the direction from
// the camera's target to its eye, 0x00584cd8) from where the camera is, then 0x00267104(camera, data, hunter) turns it
// by the hunter's camera actions (6 and 8 by the D-pad, the touch screen's pad, the Circle Pad Pro; L behind the hunter
// or, with the Target Cam on, toward its monster), and the camera's yaw (+0x9c, set with it) is then eased toward it
// (0x0026e00c). The pitch is +0x90, eased the same way into +0x98 (the same units, kept within +-0x3e94; the game turns
// it under water). The mouse's turn and the lock-on's step go into both, as MH4U's go into the input and the rig, so the
// turn shows in the same frame instead of being eased in (on land that pitch is not the camera's: its height is, the land
// camera's heights, so there the turn's up and down move the camera along them: LandHeight). The hook on 0x00267104 adds
// after the game's own turn: the mouse's turn (Hooks::AddCameraTurn) and, with the lock-on on, a part of the way toward
// the monster followed (the Target Cam's own monster, else the nearest large one in the hunter's area), every frame,
// eased, as MH4U's LockOnStep does. A turn by hand (the game's own or the mouse's) stops the following for a moment.
//
// The hunter: sPlayer [0x00c40738] + 0x20 + 4n, n the console's player number (sGame [0x00c3b978] + 0x2b, s8); its
// state block [hunter + 0xd5c]: +0x3d 3 under water, +0x56 its area of the map, +0xd8 its place (x, y, z), +0x118 its
// facing (u16), +0x14a8 the Target Cam's monster (an sEnemy slot) and +0x14a9 1 while the Target Cam is on.
// The monsters: sEnemy [0x00c1bff8] + 0x14 + 4i, 32 slots (the game clears a slot whose [unit + 4] & 7 is not 1 or 2); a
// monster's area at + 0xe3b; its place is the one 0x00853fb4(out, unit) gives, the point the game's Target Cam turns to.
//
// The end of the wait after a quest (skip_end): sQuest [0x00c409d0], its quest block Q = [sQuest + 0xaf98]: Q + 0x2c
// (s8) 2 while the quest is over and the return counts down, Q + 0x24 (s32) the wait left in 1/60 s and Q + 0x86b0 (f64)
// its real-time copy (0x0062c908 takes the time off the double and copies it while the s32 is above 0); sGame + 0x16e is
// 3 for a quest complete and waiting. The quest's update (0x00632ac8) goes on to the return once Q + 0x24 is 0 or less.
// As in MH4U, once this player has held the buttons and every port console in the session has asked too, both are set to
// 0: what running out of time does.
//
// The rewards screen (sResult [0x00c41b30], its page D = sResult + 0x14): D + 0 and D + 1 are 1 while a page runs, D +
// 0x2ff4 the page (1 and 2 the hunter's lists), D + 0x2ff8 (f32) the page's countdown in 1/60 s (7200: 2 minutes) that
// the page takes the frame's time off and ends at 0, D + 2 the page's sub-state (0 the menu, 1 the items), D + 0x3076 the
// option chosen (1 Send to Item Box), the lists at D + 0x30bc and D + 0x317c (48 x {u16 item, s16 count}).
#include "mh3u_qol.h"

#include "hooks.h"
#include "platform.h"
#ifndef __ANDROID__
#include "presence.h"  // (the PC app's Discord Rich Presence: the readings of an MH3U game)
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <fmt/format.h>

extern "C" {
#include "recomp.h"
}

namespace Port::Hooks {
// (hooks.cpp: the state the app reads through hooks.h, set here for MH3U)
void Mh3uTakeCameraTurn(double* yaw_degrees, double* pitch_degrees);
void Mh3uLockFollowed();
void Mh3uTargetCam(int monster);
void Mh3uEndSkip(bool offered, float hold, bool requested);
std::uint32_t Mh3uHostTriggers();
bool Mh3uTakeTargetTap();
}  // namespace Port::Hooks

namespace Port::Mh3uQol {

namespace {

// ── The game's addresses (MH3U USA v1.0)
constexpr u32 SGAME = 0x00c3b978;          // sGame*
constexpr u32 SPLAYER = 0x00c40738;        // sPlayer*: the hunters' units at +0x20, by player number
constexpr u32 SENEMY = 0x00c1bff8;         // sEnemy*: 32 monster slots at +0x14
constexpr u32 SQUEST = 0x00c409d0;         // sQuest*: its quest block at [+0xaf98]
constexpr u32 SRESULT = 0x00c41b30;        // sResult*: the rewards screen
constexpr u32 SUSERDATA = 0x00c43360;      // sUserData*: the save (the item box)

constexpr u32 HOOK_CAMERA_TURN = 0x00267104;  // uGameCamera's turn by the hunter's camera actions (camera, data, hunter)
constexpr u32 HOOK_PLAYER_INPUT = 0x008afc28;  // a hunter's input step each frame (hunter, ...): its pad words and actions

constexpr u32 SGAME_PLAYER = 0x2b;         // s8: this console's player number
constexpr u32 SGAME_RESULT = 0x16e;        // u8: 3 a quest complete and its wait, 4 done; 5 and 6 failed
constexpr u32 SPLAYER_UNITS = 0x20, PLAYER_COUNT = 8;
constexpr u32 HUNTER_STATE = 0xd5c;        // the hunter's state block*
[[maybe_unused]] constexpr u32 STATE_WEAPON = 0x02;  // u8: the weapon's kind, MH4U's numbers (10 the bow: the camera's test)
constexpr u32 STATE_WATER = 0x3d;          // u8: 3 under water
constexpr u32 STATE_AREA = 0x56;           // u8: the hunter's area of the map
constexpr u32 STATE_PLACE = 0xd8;          // vec3
constexpr u32 STATE_TARGET = 0x14a8;       // u8: the Target Cam's monster (sEnemy slot)
constexpr u32 STATE_TARGET_ON = 0x14a9;    // u8: 1 the Target Cam on
constexpr u32 ENEMY_SLOTS = 0x14, ENEMY_SLOT_COUNT = 32;
constexpr u32 UNIT_KIND = 0x04;            // [unit+4] & 7: 1 or 2 a live unit (the game's test of a slot)
constexpr u32 FN_MONSTER_PLACE = 0x00853fb4;  // (out*, unit): the place the Target Cam turns to (x, y, z, 0)
constexpr u32 MONSTER_AREA = 0xe3b;        // u8 (0xd60 + 0xdb)
[[maybe_unused]] constexpr u32 MONSTER_KIND = 0xd63;       // u8: the monster's kind (0xd60 + 3: the Target Cam's icon, the carves' test)
constexpr u32 MONSTER_FLAGS = 0xdec;       // u32 (0xd60 + 0x8c): bit 0 a large monster (the Target Cam's list)
constexpr u32 DATA_PITCH = 0x90, DATA_YAW = 0x94;  // u16 angles in u32s: where the camera turns to this frame
constexpr u32 DATA_PITCH_NOW = 0x98, DATA_YAW_NOW = 0x9c;  // and where it is (eased toward those by 0x0026e00c)
constexpr int PITCH_LIMIT = 0x3e94;        // the game's own, under water
constexpr u32 SQUEST_BLOCK = 0xaf98;       // sQuest: the quest block*
constexpr u32 Q_WAIT = 0x24, Q_STATE = 0x2c, Q_COUNTING = 0x2d, Q_WAIT_REAL = 0x86b0;
constexpr u32 RESULT_PAGE = 0x14;          // sResult: the page's block (D)
constexpr u32 D_RUNNING = 0x00, D_RUNNING2 = 0x01, D_SUB = 0x02, D_PAGE = 0x2ff4, D_LEFT = 0x2ff8;
constexpr u32 D_CURSOR = 0x3066, D_OPTIONS = 0x3067;  // u8: the menu's row, its rows (3)
constexpr u32 D_OPTION = 0x3076, D_LIST1 = 0x30bc, D_LIST2 = 0x317c, D_SLOTS = 48;

// the box's calls (the rewards screen's own step for one reward, 0x00895bf4's Send to Item Box)
constexpr u32 FN_ITEM_DATA = 0x0057ab4c;   // (item): its table entry; +2 bit 0x10 an item the screen keeps itself
constexpr u32 FN_BOX_ROOM = 0x00999738;    // (sUserData, sUserData + 0x18)
constexpr u32 FN_BOX_FITS = 0x0099aa60;    // (sUserData, item, sUserData + 0x1a0, room): how many go in
constexpr u32 FN_BOX_ADD = 0x0099a050;     // (sUserData, item, count, out*)
constexpr u32 FN_SOUND = 0x002d44c8;       // (sound*, number)
constexpr u32 FN_QUEST_COMPLETE = 0x0062a374;  // (sQuest): the quest complete, offline (tests only)
constexpr u32 SOUND_SENT = 8;

// the places (the area stack: sArea [0x00ca8910], +0x3818 the areas in use, +0x3820 their cArea*, the root first; an
// area's class by its vtable)
constexpr u32 SAREA = 0x00ca8910, AREA_COUNT = 0x3818, AREA_STACK = 0x3820, AREA_SLOTS = 8;
[[maybe_unused]] constexpr u32 SSTAGE = 0x00c42554;         // sStage*: +0xed the stage loaded, +0xee the hunter's area (0 the camp)
[[maybe_unused]] constexpr u32 SGUI = 0x00ca8924;           // sGUI*: +0x1c (s8) the game's language, 1 eng 2 fre 3 spa 4 ger 5 ita
[[maybe_unused]] constexpr u32 STAGE_NUMBER = 0xed, STAGE_AREA = 0xee, GUI_LANGUAGE = 0x1c;
// sUserData + 0x18 holds the save's data from its byte 4 on (+0x14 points to it): the hunter's name at the save's 7
[[maybe_unused]] constexpr u32 USER_NAME = 0x1b, USER_NAME_BYTES = 24;
// the Hunter Rank: u16 at sUserData + 0x5a38 (the save's + 0x5a24; 0 until the Guild registers the hunter in Port Tanzia,
// then 1...). The game's own reads: `add r1, r1, #0x5a00; ldrh r1, [r1, #0x38]` from [0x00c43360] at 0x005f8544,
// 0x005dd9fc and 0x005cc16c (each compares it with 6), 0x005e6d34 (with 8). Its HR points: u32 at + 0x5a34 (210 after
// The Fisherman's Tale)
[[maybe_unused]] constexpr u32 USER_HR = 0x5a38;
[[maybe_unused]] constexpr u32 SGAME_QUEST_ID = 0x44;  // sGame + 0x44 (s32): the quest chosen and played (its file's number)
enum Place { PLACE_NONE, PLACE_MENUS, PLACE_VILLAGE, PLACE_PORT, PLACE_WOODS, PLACE_QUEST };
struct AreaClass {
    u32 vtable;
    Place place;
};
constexpr AreaClass AREA_CLASSES[] = {
    {0x00b6892c, PLACE_VILLAGE}, {0x00b54fb4, PLACE_VILLAGE}, {0x00b51248, PLACE_VILLAGE}, {0x00b52f00, PLACE_VILLAGE},
    {0x00b67d10, PLACE_PORT},    {0x00b51210, PLACE_PORT},    {0x00b52ec8, PLACE_PORT},
    {0x00b4cd04, PLACE_WOODS},   {0x00b54f7c, PLACE_WOODS},
    {0x00b63bc8, PLACE_QUEST},   {0x00b4cd3c, PLACE_QUEST},   {0x00b67d80, PLACE_QUEST},  // (aResult: the quest's end)
    {0x00b636dc, PLACE_MENUS},   {0x00b4cdac, PLACE_MENUS},   {0x00b6374c, PLACE_MENUS},  {0x00b67d48, PLACE_MENUS},
    {0x00b4cccc, PLACE_MENUS},   {0x00b4f6c0, PLACE_MENUS},   {0x00b511d8, PLACE_MENUS},  {0x00b511a0, PLACE_MENUS},
    {0x00b4cd74, PLACE_MENUS}};

// gathering (sPop [0x00c40768]: 128 points at +0x41488, 0x108 bytes each; the hunter's gather action 0x0092d6dc)
constexpr u32 SPOP = 0x00c40768;
constexpr u32 SAPPPAD = 0x00c1bcb0;        // sAppPad*: +0xd4 the buttons held, +0xdc those pressed this frame
constexpr u32 PAD_HELD = 0xd4, PAD_PRESSED = 0xdc, PAD_A = 0x20;
constexpr u64 PAD_ZL = 1ull << 32, PAD_ZR = 1ull << 33;  // (HeldButtons: the app's ZL and ZR above the game's word)
constexpr u32 POINTS = 0x41488, POINT_SIZE = 0x108, POINT_COUNT = 128;
constexpr u32 POINT_USED = 0x00, POINT_ON = 0x01, POINT_AREA = 0x05, POINT_GATE = 0x07, POINT_TABLE = 0xb8;
constexpr u32 POINT_LEFT = 0xc6, POINT_SURE = 0xca;  // u16 each; 0xffff left: endless
constexpr u32 STATE_STEP_HELD = 0x09;      // u8: 1 the hunter holds the item up
constexpr u32 STATE_MODE = 0x3e, STATE_ACTION = 0x40;  // u8 the action's mode, u16 the action
constexpr u32 STATE_POUCH = 0x4d0, POUCH_SLOTS = 24, STATE_AMMO = 0x53c, AMMO_SLOTS = 8;  // {u16 item, u16 count}
constexpr u32 STATE_TOOL = 0x564;          // u16: the tool in use (pickaxes, bug nets)
constexpr u32 STATE_TABLE = 0x9b4;         // u32: the gathering point's item table the action is at
constexpr u32 FN_POINT_COUNT = 0x0054585c; // (sPop, area, &place, table): 1 a gather, 2 the last one, 0 / 0xff none
constexpr u32 FN_POINT_ROLL = 0x00546d58;  // (sPop, table): the item
constexpr u32 FN_GIVE = 0x008cf7b4;        // (hunter, type, r, item): the tool's use, the skills, the pouch, the message
constexpr u32 FN_IS_LOCAL = 0x00a39894;    // (hunter): this console's
struct GatherAction {
    u16 action;
    u8 type;
};
// mode 0's gathering actions and the type each passes to 0x0092d6dc (0x7f, a quest's object, is not a point)
constexpr GatherAction GATHER_ACTIONS[] = {{0x2c, 0}, {0x2d, 1}, {0x31, 2}, {0x32, 3}, {0xc7, 3}, {0xc8, 3},
                                            {0x33, 4}, {0x34, 5}, {0x61, 6}, {0x62, 7}, {0x63, 8}};
constexpr u16 NEUTRAL_ACTIONS[] = {0x00, 0x16, 0x1f, 0x6a};  // mode 0's neutrals (0x008fd868)

// ── Options and module state
struct Options {
    bool hold_gather = false, flex_cancel = false, sharpen_cancel = false, rewards_no_limit = false;
    bool rewards_send_all = false, gather_all = false;
    u64 skip_end = PAD_ZL | PAD_ZR;  // (PadBits)
    u64 target = PAD_ZL;             // the Target Cam's next monster (PadBits; only ZL or ZR, the app's)
    float skip_end_hold = 1.0f;
    bool trace = false, camera_trace = false;
    bool test_complete = false;  // (tests, mh3u_test_complete: the quest played now completes, as the game completes it)
    bool lock_any = false;       // (tests, mh3u_lock_any: the lock-on follows small monsters too)
    bool gather_trace = false;   // (tests, mh3u_gather_trace: the gathering points of the hunter's area, every 2 s)
    bool aim_trace = false;      // (research, mh3u_aim_trace: AimTrace)
    bool eat_trace = false;      // (research, mh3u_eat_trace: EatTrace)
};
Options options;
bool active = false;
Platform* platform = nullptr;
void (*log_fn)(const std::string&) = nullptr;

void Log(const std::string& line) {
    if (log_fn) log_fn(line);
}

bool IsOn(const std::string& v) {
    return v == "1" || v == "on" || v == "true" || v == "yes";
}

/// "ZL+ZR", "L+R", "off": the buttons' bits as HeldButtons has them: sAppPad's word (+0xd4, seen in a run: A 0x20, L 0x8,
/// R 0x80, the D-pad 0x400 right, 0x800 left, 0x1000 down, 0x2000 up) and the app's ZL and ZR above it.
u64 PadBits(std::string v) {
    static constexpr std::pair<std::string_view, u64> names[] = {
        {"A", 0x20},      {"L", 0x8},       {"R", 0x80},     {"RIGHT", 0x400}, {"LEFT", 0x800},
        {"DOWN", 0x1000}, {"UP", 0x2000},   {"ZL", PAD_ZL},  {"ZR", PAD_ZR}};
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    u64 bits = 0;
    size_t start = 0;
    while (start <= v.size()) {
        const size_t plus = v.find('+', start);
        const std::string_view name = std::string_view(v).substr(start, plus == std::string::npos ? std::string::npos : plus - start);
        for (const auto& [n, b] : names) {
            if (n == name) bits |= b;
        }
        if (plus == std::string::npos) break;
        start = plus + 1;
    }
    return bits;
}

float ReadF32(RCtx* ctx, u32 a) {
    const u32 v = rd32(ctx, a);
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}

void WriteF32(RCtx* ctx, u32 a, float f) {
    u32 v;
    std::memcpy(&v, &f, 4);
    wr32(ctx, a, v);
}

/// A pointer to the game's objects: the application heap, or the linear heap (MH3U makes its units and areas there:
/// the hunter at 0x167e1460 in a run).
bool HeapPointer(u32 p) {
    return (p & 3) == 0 && ((p >= 0x08000000 && p < 0x10000000) || (p >= 0x14000000 && p < 0x1c000000) ||
                            (p >= 0x30000000 && p < 0x38000000));
}

s64 SteadyNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

double GameSeconds() {
    return platform ? platform->GameSeconds() : 0.0;
}

/// Calls a game function with up to four arguments, the caller's registers and flags kept; its r0 (hooks.cpp's).
u32 CallGame(RCtx* ctx, u32 function, std::initializer_list<u32> args) {
    u32 regs[16];
    std::memcpy(regs, ctx->r, sizeof regs);
    const u32 flags[4] = {ctx->fn, ctx->fz, ctx->fc, ctx->fv};
    const u32 expect_next = ctx->expect_next;
    ctx->expect_next = 0;
    int i = 0;
    for (const u32 a : args) ctx->r[i++] = a;
    ctx->r[14] = function;  // (any address: the call ends when the function returns)
    rc_call(ctx, function);
    const u32 result = ctx->r[0];
    std::memcpy(ctx->r, regs, sizeof regs);
    ctx->fn = flags[0], ctx->fz = flags[1], ctx->fc = flags[2], ctx->fv = flags[3];
    ctx->expect_next = expect_next;
    return result;
}

/// u16 angle units of the vector (x, z), as 0x00584cd8.
int AngleUnits(float x, float z) {
    return static_cast<int>(std::lround(std::atan2(x, z) * 65536.0f / 6.28318531f)) & 0xffff;
}

u32 AddPitch(u32 pitch, int delta) {
    const int now = static_cast<s16>(static_cast<u16>(pitch));
    return static_cast<u16>(std::clamp(now + delta, -PITCH_LIMIT, PITCH_LIMIT));
}

/// This console's hunter (its unit), or 0.
u32 LocalHunter(RCtx* ctx) {
    const u32 game = rd32(ctx, SGAME), players = rd32(ctx, SPLAYER);
    if (!HeapPointer(game) || !HeapPointer(players)) return 0;
    const s8 n = static_cast<s8>(rd8(ctx, game + SGAME_PLAYER));
    if (n < 0 || static_cast<u32>(n) >= PLAYER_COUNT) return 0;
    const u32 hunter = rd32(ctx, players + SPLAYER_UNITS + 4u * static_cast<u32>(n));
    return HeapPointer(hunter) && HeapPointer(rd32(ctx, hunter + HUNTER_STATE)) ? hunter : 0;
}

/// sEnemy's slot `i` if it holds a live unit (the game's own test), else 0.
u32 Monster(RCtx* ctx, u32 enemies, u32 i) {
    const u32 unit = rd32(ctx, enemies + ENEMY_SLOTS + 4 * i);
    if (!HeapPointer(unit)) return 0;
    const u32 kind = rd32(ctx, unit + UNIT_KIND) & 7;
    return kind == 1 || kind == 2 ? unit : 0;
}

// ── The camera: the mouse's turn and the lock-on (the hook on 0x00267104) ──

constexpr float YAW_SIGN = -1.0f, PITCH_SIGN = 1.0f;  // (the mouse to the right turns the view right, as MH4U's)
constexpr float UNITS_PER_DEGREE = 65536.0f / 360.0f;
constexpr float LOCK_EYE = 170.0f;         // the hunter's eye above its feet (as MH4U's)
constexpr float LOCK_AIM = 90.0f;          // the camera aims this far above the monster's place
constexpr float LOCK_TAU = 0.12f;          // seconds to turn ~63% of the way
constexpr float LOCK_RANGE = 6000.0f;      // a monster farther than this is not followed
constexpr float LOCK_NEAR0 = 120.0f, LOCK_NEAR1 = 380.0f;  // right under the monster the camera holds still
constexpr s64 LOCK_AFTER_TURN = 600'000'000;  // ns after a turn by hand before following again
float rest_yaw = 0.0f, rest_pitch = 0.0f;  // angle units' fractions left over
s64 lock_hand_at = 0;
u32 lock_monster = 0;
double lock_time = 0;

// Under water the hunter swims up or down along the camera's pitch only while the game takes that pitch as held: its
// own D-pad turn there (0x002699c0) sets the data's + 0x110 to 1 and + 0x114 / + 0x118 (f32) to 30, a countdown the
// camera's step then takes the frame's time off. A pitch turned here (the mouse, a controller's right stick, the camera
// keys, a drag on the touch screen, the lock-on) is marked the same way, or the camera looked down while the hunter
// swam on level (seen in MH3U.exe: the camera key held down and the stick forward, the hunter stayed at the surface;
// marked, it dove from -124 to -690 in 1.6 s, and with the lock-on on a Gobul below it swam down toward it).
constexpr u32 DATA_PITCH_HELD = 0x110, DATA_PITCH_HOLD = 0x114, DATA_PITCH_HOLD_AT = 0x118;
constexpr float PITCH_HOLD = 30.0f;  // (the D-pad's own)
void HoldPitch(RCtx* ctx, u32 data) {
    wr8(ctx, data + DATA_PITCH_HELD, 1);
    WriteF32(ctx, data + DATA_PITCH_HOLD, PITCH_HOLD);
    WriteF32(ctx, data + DATA_PITCH_HOLD_AT, PITCH_HOLD);
}

/// A monster's place as the game's Target Cam takes it (0x00853fb4, its out* a block below the guest's stack).
bool MonsterPlace(RCtx* ctx, u32 unit, float place[3]) {
    const u32 sp = ctx->r[13];
    const u32 out = (sp - 0x100) & ~7u;
    ctx->r[13] = out;
    CallGame(ctx, FN_MONSTER_PLACE, {out, unit});
    ctx->r[13] = sp;
    for (int i = 0; i < 3; ++i) place[i] = ReadF32(ctx, out + 4 * i);
    return std::isfinite(place[0]) && std::isfinite(place[1]) && std::isfinite(place[2]);
}

/// The monster to follow: the Target Cam's own when it is on, else the nearest one in the hunter's area that the game
/// would let the Target Cam choose (a large one: LargeMonster), or 0.
bool LargeMonster(RCtx* ctx, u32 unit);
bool Targetable(RCtx* ctx, u32 unit);
u32 LockTarget(RCtx* ctx, u32 state, float hx, float hz) {
    const u32 enemies = rd32(ctx, SENEMY);
    if (!HeapPointer(enemies)) return 0;
    const u8 area = static_cast<u8>(rd8(ctx, state + STATE_AREA));
    if (rd8(ctx, state + STATE_TARGET_ON) == 1) {
        const u32 slot = rd8(ctx, state + STATE_TARGET);
        const u32 unit = slot < ENEMY_SLOT_COUNT ? Monster(ctx, enemies, slot) : 0;
        if (unit && rd8(ctx, unit + MONSTER_AREA) == area && Targetable(ctx, unit)) return unit;
    }
    u32 best = 0;
    float best_d = LOCK_RANGE * LOCK_RANGE, current_d = -1;
    for (u32 i = 0; i < ENEMY_SLOT_COUNT; ++i) {
        const u32 unit = Monster(ctx, enemies, i);
        if (!unit || rd8(ctx, unit + MONSTER_AREA) != area || !LargeMonster(ctx, unit) || !Targetable(ctx, unit)) continue;
        float place[3];
        if (!MonsterPlace(ctx, unit, place)) continue;
        const float dx = place[0] - hx, dz = place[2] - hz;
        const float d = dx * dx + dz * dz;
        if (!std::isfinite(d)) continue;
        if (unit == lock_monster && d < LOCK_RANGE * LOCK_RANGE) current_d = d;
        if (d < best_d) best_d = d, best = unit;
    }
    if (current_d >= 0 && best != lock_monster && best_d >= current_d * 0.49f) return lock_monster;
    return best;
}

/// The lock-on's step after the game's own turn: toward the monster, a part of the way.
void LockOnStep(RCtx* ctx, u32 data, u32 hunter, bool turned_by_hand) {
    const s64 now = SteadyNs();
    if (turned_by_hand) lock_hand_at = now;
    const double time = GameSeconds();
    const float dt = std::clamp(static_cast<float>(time - lock_time), 0.0f, 0.1f);
    lock_time = time;
    const u32 state = HeapPointer(hunter) ? rd32(ctx, hunter + HUNTER_STATE) : 0;
    if (HeapPointer(state)) {
        Hooks::Mh3uTargetCam(rd8(ctx, state + STATE_TARGET_ON) == 1 ? rd8(ctx, state + STATE_TARGET) + 1 : 0);
    }
    if (!Hooks::LockOn() || now - lock_hand_at < LOCK_AFTER_TURN || !HeapPointer(state)) return;
    const float hx = ReadF32(ctx, state + STATE_PLACE), hy = ReadF32(ctx, state + STATE_PLACE + 4),
                hz = ReadF32(ctx, state + STATE_PLACE + 8);
    const u32 monster = LockTarget(ctx, state, hx, hz);
    if (monster != lock_monster) {
        Log(monster ? fmt::format("lock-on: monster {:08X}", monster)
                    : std::string("lock-on: no monster to follow (still on: it waits for one)"));
        lock_monster = monster;
    }
    float place[3];
    if (!monster || dt <= 0 || !MonsterPlace(ctx, monster, place)) return;
    const float mx = place[0], my = place[1], mz = place[2];
    const float dx = hx - mx, dz = hz - mz, distance = std::sqrt(dx * dx + dz * dz);
    if (!std::isfinite(distance) || !std::isfinite(my) || !std::isfinite(hy)) return;
    const float near = std::clamp((distance - LOCK_NEAR0) / (LOCK_NEAR1 - LOCK_NEAR0), 0.0f, 1.0f);
    const float k = (1.0f - std::exp(-dt / LOCK_TAU)) * near;
    const int yaw_now = static_cast<int>(rd32(ctx, data + DATA_YAW) & 0xffff);
    const int yaw = static_cast<int>(std::lround(static_cast<s16>(static_cast<u16>(AngleUnits(dx, dz) - yaw_now)) * k));
    if (yaw != 0) {
        for (const u32 field : {DATA_YAW, DATA_YAW_NOW}) wr32(ctx, data + field, static_cast<u16>(rd32(ctx, data + field) + yaw));
    }
    if (rd8(ctx, state + STATE_WATER) == 3) {  // (under water the camera's pitch is free: toward the monster too)
        // (a monster right below or above the hunter - a Gobul lurking on the bed under a swimming hunter, seen in
        // MH3U.exe: 96 apart on the ground and 800 below - is not "right under it": the yaw holds still there, as on
        // land, but the pitch keeps turning to it, its nearness taken with the height too)
        const float rise = hy + LOCK_EYE - (my + LOCK_AIM);
        const float near_3d = std::clamp((std::sqrt(distance * distance + rise * rise) - LOCK_NEAR0) / (LOCK_NEAR1 - LOCK_NEAR0),
                                         0.0f, 1.0f);
        const int pitch_now = static_cast<s16>(static_cast<u16>(rd32(ctx, data + DATA_PITCH)));
        const int pitch_to = std::clamp(static_cast<int>(static_cast<s16>(static_cast<u16>(AngleUnits(-rise, distance)))),
                                        -PITCH_LIMIT, PITCH_LIMIT);
        const int pitch = static_cast<int>(std::lround((pitch_to - pitch_now) * (1.0f - std::exp(-dt / LOCK_TAU)) * near_3d));
        if (pitch != 0) {
            for (const u32 field : {DATA_PITCH, DATA_PITCH_NOW}) wr32(ctx, data + field, AddPitch(rd32(ctx, data + field), pitch));
            HoldPitch(ctx, data);  // (the stick forward swims toward the monster, up or down too)
        }
    }
    Hooks::Mh3uLockFollowed();
}

// ── The camera's height on land: the turn's up and down (the drag, the mouse, a controller's right stick, the keys) ──
// On land MH3U's camera has no pitch: the data's + 0x90 / + 0x98 turn the eye only under water (the step turns the eye's
// offset by them there, 0x00268310). On land the eye is the hunter's feet plus an offset (0, height, distance) turned by
// the yaw (0x00268094), the height and the distance taken from a table of the camera's heights (0x00c3ba80, 40 bytes an
// entry: + 4 the eye's height over the feet, + 8 its distance behind, + 0x14 the height it looks at, 0x00267eb0 and
// 0x00267f48), the entry [data + 0xac] that the step sets each frame, before this hook, from the height chosen [data +
// 0xa8] (0 to 5; the D-pad's up and down step it one height, 0x002699c0's actions 1 and 3, swapped by the game's own
// camera option, data + 0x66f bit 1). So the port's turn up and down did nothing on land (the S25 Ultra's 1.0.2, and
// MH3U.exe's mouse): only its yaw showed. Here, on land, it moves the camera smoothly along those heights, as MH4U's
// pitch: a height kept here between 0 and 5, the game's own the nearest whole one (the D-pad steps it as before), and
// that one's entry holding, for this frame, the values the heights have at the height kept (between it and the next
// one); the entry changed last frame gets the game's own back first. A turn of so many degrees moves the eye's angle
// over the hunter by as many degrees (the heights' angles taken between them), the way the game's camera up and down go.
constexpr u32 DATA_HEIGHT = 0xa8;          // u8: the land camera's height chosen, 0..5
constexpr u32 DATA_HEIGHT_ENTRY = 0xac;    // its entry in the table (the step's: table + 40 * height on land)
constexpr u32 HEIGHTS = 0x00c3ba80;        // the table of the land camera's heights (0x00267880's literal)
constexpr int HEIGHT_COUNT = 6;
constexpr u32 HEIGHT_SIZE = 40, HEIGHT_EYE = 0x04, HEIGHT_DISTANCE = 0x08, HEIGHT_LOOK = 0x14;
constexpr u32 HEIGHT_FIELDS[3] = {HEIGHT_EYE, HEIGHT_DISTANCE, HEIGHT_LOOK};
constexpr float HEIGHT_DEGREES = 12.0f;    // a height a turn of so many degrees when the heights' angles don't tell
// (the heights as MH3U USA v1.0 sets them: eye 400, 370, 270, 150, 60 over distances 220 to 490, looking at 170 to 210:
// the eye's angle over the hunter 43, 24, 11.5, -4.7 and -18 degrees, the camera lower and looking up down the table)
constexpr float HEIGHT_UP = 1.0f;          // a turn up (+) goes down the table (+1: the eye lower, looking up), as the
                                           // game's camera up does on land (action 1) and its pitch up under water
struct LandHeights {
    bool read = false, ok = false;         // the table's values read (once: the game sets them at its start)
    float v[HEIGHT_COUNT][3] = {};         // each height's eye, distance and look
    float angle[HEIGHT_COUNT] = {};        // the eye's angle over the hunter at each (degrees)
    bool by_angle = false;                 // the angles grow or shrink with the height, enough to turn by them
    float kept = -1.0f;                    // the height kept (0..5), -1 none yet
    int written = -1;                      // the entry written last frame (-1 none)
};
LandHeights land;

/// The game's own values back in the entry written last.
void LandRestore(RCtx* ctx) {
    if (land.written < 0) return;
    const u32 entry = HEIGHTS + HEIGHT_SIZE * static_cast<u32>(land.written);
    for (int f = 0; f < 3; ++f) WriteF32(ctx, entry + HEIGHT_FIELDS[f], land.v[land.written][f]);
    land.written = -1;
}

void LandRead(RCtx* ctx) {
    land.read = true;
    land.ok = true;
    for (int i = 0; i < HEIGHT_COUNT; ++i) {
        for (int f = 0; f < 3; ++f) land.v[i][f] = ReadF32(ctx, HEIGHTS + HEIGHT_SIZE * static_cast<u32>(i) + HEIGHT_FIELDS[f]);
        const float eye = land.v[i][0], distance = land.v[i][1], look = land.v[i][2];
        if (!std::isfinite(eye) || !std::isfinite(look) || !(distance > 1.0f && distance < 100000.0f)) land.ok = false;
        land.angle[i] = std::atan2(eye - look, distance) * 57.29578f;
    }
    float least = 1e9f;
    int sign = 0;
    for (int i = 1; i < HEIGHT_COUNT && land.ok; ++i) {
        const float d = land.angle[i] - land.angle[i - 1];
        const int si = d > 0.0f ? 1 : d < 0.0f ? -1 : 0;
        if (si == 0 || (sign != 0 && si != sign)) sign = 2;
        else if (sign != 2) sign = si;
        least = std::min(least, std::fabs(d));
    }
    land.by_angle = land.ok && (sign == 1 || sign == -1) && least > 0.5f;
    std::string line = land.ok ? "camera: the land camera's heights (eye, distance, look; angle):"
                               : "camera: the land camera's heights not as known (another version), left alone:";
    for (int i = 0; i < HEIGHT_COUNT; ++i) {
        line += fmt::format(" {} {:.0f} {:.0f} {:.0f} {:.1f};", i, land.v[i][0], land.v[i][1], land.v[i][2], land.angle[i]);
    }
    Log(line + (land.by_angle ? std::string(" the turn goes by their angles")
                              : fmt::format(" a height every {:.0f} degrees", HEIGHT_DEGREES)));
}

/// The eye's angle at a height between two (the heights' values taken between them).
float LandAngle(float height) {
    const int i = std::clamp(static_cast<int>(std::floor(height)), 0, HEIGHT_COUNT - 2);
    const float t = std::clamp(height - static_cast<float>(i), 0.0f, 1.0f);
    return land.angle[i] + (land.angle[i + 1] - land.angle[i]) * t;
}

/// The height whose angle is `angle` (by_angle: the angles grow or shrink with the height), within 0..5.
float LandHeightAt(float angle) {
    const float first = land.angle[0], last = land.angle[HEIGHT_COUNT - 1];
    angle = std::clamp(angle, std::min(first, last), std::max(first, last));
    for (int i = 0; i + 1 < HEIGHT_COUNT; ++i) {
        const float a0 = land.angle[i], a1 = land.angle[i + 1];
        if (angle >= std::min(a0, a1) && angle <= std::max(a0, a1) && a1 != a0) {
            return static_cast<float>(i) + (angle - a0) / (a1 - a0);
        }
    }
    return angle == first ? 0.0f : static_cast<float>(HEIGHT_COUNT - 1);
}

/// On land, after the game's own step of the camera's height: the turn up and down (degrees, + up) moved along the
/// heights, and the entry the step chose written for this frame. False under water (the turn's pitch is the data's
/// there); true on land, where the turn's up and down never go to the data's pitch (a camera of the game's own, riding
/// or a cutscene's, or a table not as known: they do nothing there, as before).
bool LandHeight(RCtx* ctx, u32 data, u32 state, double up_degrees) {
    LandRestore(ctx);
    if (!HeapPointer(state)) {
        land.kept = -1.0f;
        return true;
    }
    if (rd8(ctx, state + STATE_WATER) == 3) {
        land.kept = -1.0f;  // (back on land the game's own height again)
        return false;
    }
    const int height = static_cast<int>(rd8(ctx, data + DATA_HEIGHT));
    if (height >= HEIGHT_COUNT || rd32(ctx, data + DATA_HEIGHT_ENTRY) != HEIGHTS + HEIGHT_SIZE * static_cast<u32>(height)) {
        land.kept = -1.0f;
        return true;
    }
    if (!land.read) LandRead(ctx);
    if (!land.ok) return true;
    // the height kept follows the game's own steps (the D-pad's, its own choices): within half a height of it
    if (land.kept < 0.0f || std::fabs(land.kept - static_cast<float>(height)) > 0.5f) land.kept = static_cast<float>(height);
    const float before = land.kept;
    if (up_degrees != 0.0) {
        const float turn = static_cast<float>(up_degrees) * HEIGHT_UP;  // (+: down the table)
        float to;
        if (land.by_angle) {
            const float grows = land.angle[HEIGHT_COUNT - 1] > land.angle[0] ? 1.0f : -1.0f;  // (the angle with the height)
            to = LandHeightAt(LandAngle(land.kept) + turn * grows);
        } else {
            to = land.kept + turn / HEIGHT_DEGREES;
        }
        land.kept = std::clamp(to, 0.0f, static_cast<float>(HEIGHT_COUNT - 1));
    }
    // the game's own height the nearest whole one (its D-pad steps from there), and this frame's entry at the height kept
    const int whole = std::clamp(static_cast<int>(std::lround(land.kept)), 0, HEIGHT_COUNT - 1);
    if (whole != height) {
        wr8(ctx, data + DATA_HEIGHT, static_cast<u8>(whole));
        wr32(ctx, data + DATA_HEIGHT_ENTRY, HEIGHTS + HEIGHT_SIZE * static_cast<u32>(whole));
    }
    if (std::fabs(land.kept - static_cast<float>(whole)) > 1e-4f) {
        const int i = std::clamp(static_cast<int>(std::floor(land.kept)), 0, HEIGHT_COUNT - 2);
        const float t = std::clamp(land.kept - static_cast<float>(i), 0.0f, 1.0f);
        const u32 entry = HEIGHTS + HEIGHT_SIZE * static_cast<u32>(whole);
        for (int f = 0; f < 3; ++f) WriteF32(ctx, entry + HEIGHT_FIELDS[f], land.v[i][f] + (land.v[i + 1][f] - land.v[i][f]) * t);
        land.written = whole;
    }
    if (options.camera_trace) {  // (research: the eye the step worked out last frame, 0x00268094's, and the eye it shows)
        static int after = 0, n = 0;
        if (land.kept != before) after = 12;
        if (after > 0 || (++n % 60) == 0) {
            if (after > 0) --after;
            Log(fmt::format("mh3u camera: data {:08x} height {} entry {:08x} kept {:.2f}; eye to {:.0f} {:.0f} {:.0f} at {:.0f} {:.0f} "
                            "{:.0f}, look {:.0f} {:.0f} {:.0f}; c0 {} 10e {} hunter {:.0f} {:.0f} {:.0f}",
                            data, rd8(ctx, data + DATA_HEIGHT), rd32(ctx, data + DATA_HEIGHT_ENTRY), land.kept,
                            ReadF32(ctx, data + 0x70), ReadF32(ctx, data + 0x74), ReadF32(ctx, data + 0x78), ReadF32(ctx, data),
                            ReadF32(ctx, data + 4), ReadF32(ctx, data + 8), ReadF32(ctx, data + 0x80), ReadF32(ctx, data + 0x84),
                            ReadF32(ctx, data + 0x88), rd8(ctx, data + 0xc0), static_cast<s16>(rd16(ctx, data + 0x10e)),
                            ReadF32(ctx, state + STATE_PLACE), ReadF32(ctx, state + STATE_PLACE + 4),
                            ReadF32(ctx, state + STATE_PLACE + 8)));
        }
    }
    if (options.camera_trace && land.kept != before) {
        Log(fmt::format("mh3u camera: on land the turn {:+.1f} degrees up: height {} -> {:.2f} (the game's {})", up_degrees,
                        height, land.kept, whole));
    }
    return true;
}

void CameraTurn(RCtx* ctx, RFunc original) {
    const u32 data = ctx->r[1];
    const u32 hunter = ctx->r[2];
    const u32 before = HeapPointer(data) ? (rd32(ctx, data + DATA_YAW) & 0xffff) | rd32(ctx, data + DATA_PITCH) << 16 : 0;
    original(ctx);
    const u32 r0 = ctx->r[0];
    if (!HeapPointer(data) || hunter != LocalHunter(ctx)) {
        ctx->r[0] = r0;
        return;
    }
    const u32 after = (rd32(ctx, data + DATA_YAW) & 0xffff) | rd32(ctx, data + DATA_PITCH) << 16;
    const bool game_turned = after != before;  // (the D-pad, the touch screen's pad, the Circle Pad Pro, L)
    double yaw_degrees = 0, pitch_degrees = 0;
    Hooks::Mh3uTakeCameraTurn(&yaw_degrees, &pitch_degrees);
    // (on land the turn's up and down go to the camera's height: LandHeight; under water to the data's pitch)
    const bool on_land = LandHeight(ctx, data, rd32(ctx, hunter + HUNTER_STATE), pitch_degrees * PITCH_SIGN);
    const bool land_turned = on_land && pitch_degrees != 0;
    if (on_land) pitch_degrees = 0, rest_pitch = 0.0f;
    const float yaw_units = static_cast<float>(yaw_degrees) * UNITS_PER_DEGREE * YAW_SIGN + rest_yaw;
    const float pitch_units = static_cast<float>(pitch_degrees) * UNITS_PER_DEGREE * PITCH_SIGN + rest_pitch;
    const int yaw = static_cast<int>(yaw_units), pitch = static_cast<int>(pitch_units);
    rest_yaw = yaw_units - static_cast<float>(yaw);
    rest_pitch = pitch_units - static_cast<float>(pitch);
    if (yaw != 0) {
        for (const u32 field : {DATA_YAW, DATA_YAW_NOW}) wr32(ctx, data + field, static_cast<u16>(rd32(ctx, data + field) + yaw));
    }
    if (pitch != 0) {
        for (const u32 field : {DATA_PITCH, DATA_PITCH_NOW}) wr32(ctx, data + field, AddPitch(rd32(ctx, data + field), pitch));
        const u32 state = rd32(ctx, hunter + HUNTER_STATE);
        if (HeapPointer(state) && rd8(ctx, state + STATE_WATER) == 3) HoldPitch(ctx, data);
    }
    LockOnStep(ctx, data, hunter, game_turned || yaw != 0 || pitch != 0 || land_turned);
    if (options.camera_trace && (game_turned || yaw != 0 || pitch != 0)) {
        Log(fmt::format("mh3u camera: yaw {:04x} -> {:04x} -> {:04x}, pitch {:04x} -> {:04x} (mouse {} {})", before & 0xffff,
                        after & 0xffff, rd32(ctx, data + DATA_YAW) & 0xffff, before >> 16, rd32(ctx, data + DATA_PITCH) & 0xffff,
                        yaw, pitch));
    }
    ctx->r[0] = r0;
}

// ── Aiming a bowgun: the camera's turn (a drag, the mouse, the gyro) turns the hunter's aim ──
// MH3U aims a bowgun by turning the hunter: R held with a bowgun out puts the hunter's camera in its aiming mode
// (sGameCamera + 0x20 == 2, the red cross), and that camera is built from the hunter (0x009181bc: the state block's
// angles at + 0x114, its facing at + 0x118, and the aim's height, + 0xe77: an s8 from -100 to 100, its share of 45
// degrees), not from uGameCamera's yaw. While aiming the Circle Pad turns the facing (unit + 0xed8, the one the game
// takes, as the trace showed: the stick right took it down 0x300 a frame, + 0x118 following) and raises or lowers
// + 0xe77. So while the hunter aims, the turn the app asks for (Hooks::AddCameraTurn: a drag, the mouse, the phone's
// gyro: MH4U's gyro aim turns the camera, which is MH4U's aim) is taken here, at the frame's start, and added to the
// facing (and the copies the game makes of it, as hunt_aids' Mh3uSetFacing) and to the aim's height. Taking it marks
// the turn as used (CameraTurnActive), so the app keeps sending the drag's turn as degrees.
constexpr u32 UNIT_FACING = 0xed8;
constexpr u32 UNIT_FACING_COPIES[] = {0xc8c, 0xe78, 0xf1c, 0xf1e};
constexpr u32 SGAMECAMERA = 0x00c3b9cc, GAMECAMERA_MODE = 0x20;  // sGameCamera*: + 0x20 the camera's mode
constexpr u32 CAMERA_AIMING = 2;           // (the mode while a bowgun aims: R held)
constexpr u32 STATE_AIM_HEIGHT = 0xe77;    // s8: -100..100, its share of 45 degrees (0x009181bc)
constexpr float AIM_HEIGHT_DEGREES = 45.0f;
struct AimFields {
    float yaw_sign = -1.0f;    // (a turn to the right takes the facing down, as the stick right does)
    float pitch_sign = 1.0f;   // (up raises + 0xe77: at 44 the Moto's aim looked up, at -44 down; mh3u_aim_sign "yaw,pitch")
    bool trace = false;        // (mh3u_aim_log: a line for each turn taken, not only the aim's first)
};
AimFields aim;
u32 aim_logged = 0;
float aim_height_rest = 0.0f;

bool Aiming(RCtx* ctx, u32 state) {
    const u8 weapon = static_cast<u8>(rd8(ctx, state + STATE_WEAPON));
    if ((weapon != 4 && weapon != 6) || rd8(ctx, state + 0x58) == 0) return false;  // (a bowgun, out)
    const u32 gamecam = rd32(ctx, SGAMECAMERA);
    return HeapPointer(gamecam) && rd8(ctx, gamecam + GAMECAMERA_MODE) == CAMERA_AIMING;
}

void AimStep(RCtx* ctx) {
    const u32 hunter = LocalHunter(ctx);
    if (!hunter) return;
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    if (!Aiming(ctx, state)) {
        aim_logged = 0, aim_height_rest = 0.0f;
        return;
    }
    double yaw_degrees = 0, pitch_degrees = 0;
    Hooks::Mh3uTakeCameraTurn(&yaw_degrees, &pitch_degrees);  // (the app's turn goes to the aim now: CameraTurnActive)
    if (yaw_degrees == 0 && pitch_degrees == 0) return;
    const int yaw = static_cast<int>(std::lround(yaw_degrees * UNITS_PER_DEGREE * aim.yaw_sign));
    const u16 facing_before = static_cast<u16>(rd16(ctx, hunter + UNIT_FACING));
    if (yaw != 0) {
        const u16 facing = static_cast<u16>(facing_before + yaw);
        wr16(ctx, hunter + UNIT_FACING, facing);
        for (const u32 copy : UNIT_FACING_COPIES) wr16(ctx, hunter + copy, facing);
    }
    const int height_before = static_cast<s8>(rd8(ctx, state + STATE_AIM_HEIGHT));
    const float height_units = static_cast<float>(pitch_degrees) * 100.0f / AIM_HEIGHT_DEGREES * aim.pitch_sign + aim_height_rest;
    const int height_step = static_cast<int>(height_units);
    aim_height_rest = height_units - static_cast<float>(height_step);
    if (height_step != 0) {
        wr8(ctx, state + STATE_AIM_HEIGHT, static_cast<u8>(static_cast<s8>(std::clamp(height_before + height_step, -100, 100))));
    }
    if (aim.trace || aim_logged++ == 0) {
        Log(fmt::format("aim: the turn {:.1f}, {:.1f} degrees to the hunter's aim: facing {:04x} -> {:04x}, height {} -> {}",
                        yaw_degrees, pitch_degrees, facing_before, rd16(ctx, hunter + UNIT_FACING), height_before,
                        static_cast<s8>(rd8(ctx, state + STATE_AIM_HEIGHT))));
    }
}

// ── The large monsters (the lock-on's choice when the Target Cam is off) ──
// The Target Cam's panel (0x0039d278) lists sEnemy's monsters whose bit 0 of [unit + 0xd60 + 0x8c] is set: the large
// ones, which it offers to follow; the same test here. A monster the game's Target Cam would turn to also answers its
// virtual +0x144 (0x00267604): one that doesn't (gone, carved) isn't followed.
bool LargeMonster(RCtx* ctx, u32 unit) {
    return options.lock_any || (rd32(ctx, unit + MONSTER_FLAGS) & 1) != 0;
}

bool Targetable(RCtx* ctx, u32 unit) {
    const u32 vtable = rd32(ctx, unit);
    const u32 fn = vtable >= 0x00b00000 && vtable < 0x00c00000 ? rd32(ctx, vtable + 0x144) : 0;
    return fn >= 0x00100000 && fn < 0x00b00000 && CallGame(ctx, fn, {unit}) != 0;
}

// ── The hunter's input step (the hook on 0x008afc28): hold A to gather ──
// The step copies the pad's words from sAppPad (+0xd4 held, +0xdc pressed this frame) into [hunter + 0xd5c] + 0x194 and
// builds the hunter's actions from them. With qol_hold_gather, while A stays held through a gather (carve, mine, catch)
// the step sees A pressed again: at the gather's end the hunter starts the next one, as a press of A would, until A is
// let go or the hunter does something else. Only this console's hunter, and only for that step (the pad's word is put
// back after it).
int GatherType(u8 mode, u16 action) {
    if (mode != 0) return -1;
    for (const GatherAction& g : GATHER_ACTIONS) {
        if (g.action == action) return g.type;
    }
    return -1;
}

bool Neutral(u8 mode, u16 action) {
    return mode == 0 && std::find(std::begin(NEUTRAL_ACTIONS), std::end(NEUTRAL_ACTIONS), action) != std::end(NEUTRAL_ACTIONS);
}

bool hold_gathering = false;  // the hunter gathered with A held, and A is still held

// ── The flex after a drink and the sharpening: moving or a button ends them (qol_flex_cancel, qol_sharpen_cancel) ──
// MH4U's "Interruptable Flex" and "Whetstone Sharpening Cancel" (jM5557's) for MH3U. The actions, as the hunter's action
// trace showed them on a phone (the Moga Woods, a Potion and a Whetstone): a drink is mode 0's 0x35 (0x0092e54c, its
// variants 0x36-0x38 the meat and the water's), which ends in the flex, 0x39 (0x3a under water: 0x0092e88c); sharpening
// is 0x3d (0x0092ec80; 0x48 under water, 0xca its other variant). The game ends both by sending the hunter to its neutral,
// 0x008fd868(hunter, the state block's + 0x3d, 4, 0; 0, 0), when their motion is over; here that call comes early, before
// the hunter's input step reads the pad, so the same frame's input starts what the player asked for: the stick walks,
// B rolls (a roll is what MH4U's cancel of the sharpening does: the neutral takes the next frame's B, given to it again,
// as a roll, 0x1e). The flex ends on the stick or on A, B, X or Y pressed while L is not held (L with A or Y turns the
// item bar); the sharpening on B. The drink's effect has already been given when the flex starts. Seen on the Moto
// (mh3u-faltan, the Moga Woods): the stick ended the flex and the hunter walked, B ended it, and B ended the sharpening
// with a roll.
constexpr u16 FLEX_ACTIONS[] = {0x39, 0x3a};
constexpr u16 SHARPEN_ACTIONS[] = {0x3d, 0x48, 0xca};
constexpr u32 FN_NEUTRAL = 0x008fd868;  // (hunter, kind, blend, 0; [sp] 0, [sp + 4] 0)
constexpr u32 STATE_KIND = 0x3d;        // u8: the neutral the hunter's actions go back to (3 under water)
constexpr u32 PAD_B = 0x40, PAD_X = 0x100, PAD_Y = 0x200, PAD_L = 0x8;
constexpr u32 PAD_STICK = 0x3c000000;   // the circle pad's four ways (0x04000000 right, 0x20000000 up: the trace)

/// A game call with two more arguments on the stack (the stack pointer put back after it).
u32 CallGame6(RCtx* ctx, u32 function, std::initializer_list<u32> args, u32 fifth, u32 sixth) {
    const u32 sp = ctx->r[13];
    ctx->r[13] = sp - 8;
    wr32(ctx, sp - 8, fifth);
    wr32(ctx, sp - 4, sixth);
    const u32 result = CallGame(ctx, function, args);
    ctx->r[13] = sp;
    return result;
}

bool roll_pending = false;  // the sharpening ended by B: the next input step sees B pressed again, so the hunter rolls

void CancelStep(RCtx* ctx, u32 hunter, u32 pad) {
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    if (!HeapPointer(state) || rd8(ctx, state + STATE_MODE) != 0) return;
    const u16 action = static_cast<u16>(rd16(ctx, state + STATE_ACTION));
    const bool flex = options.flex_cancel && std::find(std::begin(FLEX_ACTIONS), std::end(FLEX_ACTIONS), action) !=
                                                 std::end(FLEX_ACTIONS);
    const bool sharpen = options.sharpen_cancel && std::find(std::begin(SHARPEN_ACTIONS), std::end(SHARPEN_ACTIONS),
                                                             action) != std::end(SHARPEN_ACTIONS);
    if (!flex && !sharpen) return;
    const u32 held = rd32(ctx, pad + PAD_HELD), pressed = rd32(ctx, pad + PAD_PRESSED);
    const bool go = flex ? (held & PAD_STICK) != 0 || (!(held & PAD_L) && (pressed & (PAD_A | PAD_B | PAD_X | PAD_Y)) != 0)
                         : (pressed & PAD_B) != 0;
    if (!go) return;
    CallGame6(ctx, FN_NEUTRAL, {hunter, rd8(ctx, state + STATE_KIND), 4, 0}, 0, 0);
    roll_pending = sharpen;  // (the neutral takes the B of the next input step as a roll, as MH4U's cancel rolls)
    Log(fmt::format("qol: {} (action {:02X}) ended by {}", flex ? "the flex" : "the sharpening", action,
                    (held & PAD_STICK) && flex ? "the stick" : (pressed & PAD_B) ? "B" : "a button"));
}

void PlayerInput(RCtx* ctx, RFunc original) {
    const u32 hunter = ctx->r[0];
    const u32 pad = rd32(ctx, SAPPPAD);
    if ((options.flex_cancel || options.sharpen_cancel) && HeapPointer(pad) && hunter == LocalHunter(ctx)) {
        if (roll_pending) {  // (the frame after the sharpening's cancel: B again for this step, put back after it)
            roll_pending = false;
            const u32 state = rd32(ctx, hunter + HUNTER_STATE);
            const u32 pressed = rd32(ctx, pad + PAD_PRESSED);
            if (HeapPointer(state) && Neutral(static_cast<u8>(rd8(ctx, state + STATE_MODE)),
                                              static_cast<u16>(rd16(ctx, state + STATE_ACTION))) && !(pressed & PAD_B)) {
                wr32(ctx, pad + PAD_PRESSED, pressed | PAD_B);
                original(ctx);
                wr32(ctx, pad + PAD_PRESSED, rd32(ctx, pad + PAD_PRESSED) & ~PAD_B);
                Log("qol: a roll after the sharpening");
                return;
            }
        }
        CancelStep(ctx, hunter, pad);
    }
    if (!options.hold_gather || !HeapPointer(pad) || hunter != LocalHunter(ctx)) {
        if (!options.hold_gather) hold_gathering = false;
        original(ctx);
        return;
    }
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const u8 mode = static_cast<u8>(rd8(ctx, state + STATE_MODE));
    const u16 action = static_cast<u16>(rd16(ctx, state + STATE_ACTION));
    const u32 held = rd32(ctx, pad + PAD_HELD), pressed = rd32(ctx, pad + PAD_PRESSED);
    bool inject = false;
    if (!(held & PAD_A)) {
        hold_gathering = false;
    } else if (GatherType(mode, action) >= 0) {
        if (!hold_gathering && options.trace) Log(fmt::format("qol: gathering (action {:02X}) with A held", action));
        hold_gathering = true;
        inject = true;  // (the press waits in the hunter's input: the next gather follows this one)
    } else if (!Neutral(mode, action)) {
        hold_gathering = false;
    } else if (hold_gathering) {
        inject = true;  // (back at the neutral with A still held: gather again)
    }
    if (inject && !(pressed & PAD_A)) {
        wr32(ctx, pad + PAD_PRESSED, pressed | PAD_A);
        original(ctx);
        wr32(ctx, pad + PAD_PRESSED, rd32(ctx, pad + PAD_PRESSED) & ~PAD_A);
        return;
    }
    original(ctx);
}

// ── Gather everything in one press (gather_all) ──
// At the frame point, the gathering points' counts are compared with the last frame's: when one in the hunter's area
// with the table of the gather the local hunter is in went down and the hunter's pouch changed, the game gathered there
// once in the last frame, and the same three steps the gather action takes run again on that point until it has no
// gathers left: 0x0054585c counts the gather as the game does (the sure ones first, the chance it ends, the skills),
// 0x00546d58 rolls the item and 0x008cf7b4 gives it (the tool's use and break, the skills' extra items, the pouch and the
// message). It stops at the last gather, at an item the hunter holds up, at a broken tool, and when an item didn't go
// into the pouch (full, or as many as it can carry: the point gets that gather back). Endless points are left to the game;
// only in a quest this console plays (sGame + 0x28 == 1). A point's gate (+ 0x07: 1 a timer at + 0x84, 0 and 2 the flag
// + 0x09) is the game's count's own check (0x005458f8), not a carve's mark: the Deserted Island's herbs have gate 1, so
// the gate doesn't keep a point out (mh3u-pc3's run g3: before, gather_all never fired there). A corpse's carves go too,
// as MH4U's gather_all carves them all.
struct PointCounts {
    u16 left = 0, sure = 0;
};
PointCounts point_seen[POINT_COUNT];
bool points_valid = false;
u32 points_hunter = 0;
u32 pouch_seen = 0;

u32 PouchSum(RCtx* ctx, u32 state) {
    u32 sum = 0;
    for (u32 i = 0; i < POUCH_SLOTS; ++i) sum = sum * 31 + rd32(ctx, state + STATE_POUCH + 4 * i);
    for (u32 i = 0; i < AMMO_SLOTS; ++i) sum = sum * 31 + rd32(ctx, state + STATE_AMMO + 4 * i);
    return sum;
}

/// (tests, mh3u_gather_trace) the gathering points of the hunter's area: their distance, and their bearing from the way
/// the camera looks (degrees, to the right), so that a session driven by hand finds them; and the hunter's action.
void GatherTrace(RCtx* ctx, u32 pop, u32 hunter) {
    static u32 count = 0;
    if (++count % 60 != 0 || !HeapPointer(pop) || !hunter) return;
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const u32 gamecam = rd32(ctx, 0x00c3b9cc);
    const u32 camera = HeapPointer(gamecam) ? rd32(ctx, gamecam + 0x14) : 0;
    const u32 data = HeapPointer(camera) ? rd32(ctx, camera + 0x8e8) : 0;
    const int view = HeapPointer(data) ? static_cast<int>((rd32(ctx, data + DATA_YAW_NOW) + 0x8000) & 0xffff) : 0;
    const float hx = ReadF32(ctx, state + STATE_PLACE), hz = ReadF32(ctx, state + STATE_PLACE + 8);
    const u8 area = static_cast<u8>(rd8(ctx, state + STATE_AREA));
    std::string line;
    for (u32 i = 0; i < POINT_COUNT; ++i) {
        const u32 point = pop + POINTS + POINT_SIZE * i;
        if (!rd8(ctx, point + POINT_USED) || rd8(ctx, point + POINT_AREA) != area) continue;
        const float dx = ReadF32(ctx, point + 0x98) - hx, dz = ReadF32(ctx, point + 0x98 + 8) - hz;
        const int rel = static_cast<s16>(static_cast<u16>(AngleUnits(dx, dz) - view));
        line += fmt::format(" #{} {:.0f} at {:+.0f} deg (left {} sure {} on {})", i, std::sqrt(dx * dx + dz * dz),
                            rel * 360.0 / 65536.0, rd16(ctx, point + POINT_LEFT), rd16(ctx, point + POINT_SURE),
                            rd8(ctx, point + POINT_ON));
    }
    Log(fmt::format("gather trace: area {} action {}/{:02x}{}", area, rd8(ctx, state + STATE_MODE),
                    rd16(ctx, state + STATE_ACTION), line.empty() ? std::string(" (no points here)") : line));
}

void GatherStep(RCtx* ctx) {
    const u32 pop = rd32(ctx, SPOP), game = rd32(ctx, SGAME);
    const u32 hunter = LocalHunter(ctx);
    if (options.gather_trace) GatherTrace(ctx, pop, hunter);
    if (!options.gather_all || !HeapPointer(pop) || !HeapPointer(game) || !hunter || rd8(ctx, game + 0x28) != 1) {
        points_valid = false;
        return;
    }
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const int type = GatherType(static_cast<u8>(rd8(ctx, state + STATE_MODE)), static_cast<u16>(rd16(ctx, state + STATE_ACTION)));
    const u32 pouch = PouchSum(ctx, state);
    const u8 area = static_cast<u8>(rd8(ctx, state + STATE_AREA));
    const u32 table = rd32(ctx, state + STATE_TABLE);
    int gathered = -1;
    for (u32 i = 0; i < POINT_COUNT; ++i) {
        const u32 point = pop + POINTS + POINT_SIZE * i;
        const PointCounts now{static_cast<u16>(rd16(ctx, point + POINT_LEFT)), static_cast<u16>(rd16(ctx, point + POINT_SURE))};
        if (options.gather_trace && points_valid && points_hunter == hunter && rd8(ctx, point + POINT_USED) &&
            rd8(ctx, point + POINT_AREA) == area && (now.left < point_seen[i].left || now.sure < point_seen[i].sure)) {
            Log(fmt::format("gather trace: point {} went {}/{} -> {}/{} in action {}/{:02x} (type {}), its table {}, the "
                            "pouch {}, gate {}", i, point_seen[i].left, point_seen[i].sure, now.left, now.sure,
                            rd8(ctx, state + STATE_MODE), rd16(ctx, state + STATE_ACTION), type,
                            rd32(ctx, point + POINT_TABLE) == table ? "the action's" : "another",
                            pouch != pouch_seen ? "changed" : "the same", static_cast<s8>(rd8(ctx, point + POINT_GATE))));
        }
        if (points_valid && points_hunter == hunter && type >= 0 && gathered < 0 && rd8(ctx, point + POINT_USED) &&
            rd8(ctx, point + POINT_AREA) == area && rd32(ctx, point + POINT_TABLE) == table && now.left != 0xffff &&
            (now.left < point_seen[i].left || now.sure < point_seen[i].sure) && pouch != pouch_seen) {
            gathered = static_cast<int>(i);
        }
        point_seen[i] = now;
    }
    points_valid = true, points_hunter = hunter, pouch_seen = pouch;
    if (gathered < 0 || CallGame(ctx, FN_IS_LOCAL, {hunter}) == 0) return;
    const u32 point = pop + POINTS + POINT_SIZE * static_cast<u32>(gathered);
    const u16 tool = static_cast<u16>(rd16(ctx, state + STATE_TOOL));
    int extra = 0;
    const char* why = "the point is spent";
    for (int n = 0; n < 64; ++n) {
        const u16 left = static_cast<u16>(rd16(ctx, point + POINT_LEFT)), sure = static_cast<u16>(rd16(ctx, point + POINT_SURE));
        if (!rd8(ctx, point + POINT_USED) || !rd8(ctx, point + POINT_ON) || (left == 0 && sure == 0)) break;
        if (rd8(ctx, state + STATE_STEP_HELD) == 1) {
            why = "an item held up";
            break;
        }
        if ((type == 2 || type == 3 || type == 8) && rd16(ctx, state + STATE_TOOL) != tool) {
            why = "the tool broke";
            break;
        }
        const u32 before = PouchSum(ctx, state);
        const u32 r = CallGame(ctx, FN_POINT_COUNT, {pop, area, state + STATE_PLACE, table}) & 0xff;
        if (r != 1 && r != 2) break;
        const u32 item = CallGame(ctx, FN_POINT_ROLL, {pop, table}) & 0xffff;
        CallGame(ctx, FN_GIVE, {hunter, static_cast<u32>(type), r, item});
        if (PouchSum(ctx, state) == before) {  // (it didn't go in: the point keeps that gather)
            wr16(ctx, point + POINT_LEFT, left);
            wr16(ctx, point + POINT_SURE, sure);
            why = "the pouch took no more";
            break;
        }
        ++extra;
        if (r == 2) break;
    }
    point_seen[gathered] = {static_cast<u16>(rd16(ctx, point + POINT_LEFT)), static_cast<u16>(rd16(ctx, point + POINT_SURE))};
    pouch_seen = PouchSum(ctx, state);
    Log(fmt::format("gather everything: point {} (type {}) gave {} more ({})", gathered, type, extra, why));
}

/// The buttons held now: the 3DS's as the game reads them (sAppPad + 0xd4) and the app's ZL and ZR
/// (Hooks::SetHostTriggers: the game has them only through the Circle Pad Pro).
u64 HeldButtons(RCtx* ctx) {
    const u32 pad = rd32(ctx, SAPPPAD);
    u64 held = HeapPointer(pad) ? rd32(ctx, pad + PAD_HELD) : 0;
    const u32 host = Hooks::Mh3uHostTriggers();  // (hooks.cpp's: ZL 0x400 and ZR 0x800, MH4U's pad word's)
    if (host & 0x400) held |= PAD_ZL;
    if (host & 0x800) held |= PAD_ZR;
    return held;
}

// ── The Target Cam's next monster (the port's target button, ZL; the app's tap on the target camera) ──
// The touch screen's Target Cam panel (0x0039fc70, 0x0039fdfc) chooses a large monster for the camera by its sEnemy slot,
// in the hunter's state block: +0x14a8 the slot, +0x14a9 1 on. The button steps through the large monsters as taps on
// the panel's icons do: none, the first, the next..., none again. L then turns the camera to it (the game's own), and
// the lock-on follows it.
u64 target_held = 0;

void TargetStep(RCtx* ctx) {
    const u64 bits = HeldButtons(ctx);
    const bool pressed = options.target && (bits & options.target) && !(target_held & options.target);
    target_held = bits;
    const bool tapped = Hooks::Mh3uTakeTargetTap();
    if (!pressed && !tapped) return;
    const u32 game = rd32(ctx, SGAME), enemies = rd32(ctx, SENEMY);
    const u32 hunter = LocalHunter(ctx);
    if (!hunter || !HeapPointer(game) || !HeapPointer(enemies) || rd8(ctx, game + 0x28) != 1) return;
    if (options.skip_end && (bits & options.skip_end) == options.skip_end) return;  // (the quest's end: ZL+ZR)
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const bool on = rd8(ctx, state + STATE_TARGET_ON) == 1;
    const u32 current = rd8(ctx, state + STATE_TARGET);
    int next = -1;
    for (u32 i = on ? current + 1 : 0; i < ENEMY_SLOT_COUNT; ++i) {
        const u32 unit = Monster(ctx, enemies, i);
        if (unit && LargeMonster(ctx, unit) && Targetable(ctx, unit)) {
            next = static_cast<int>(i);
            break;
        }
    }
    if (next < 0) {
        wr8(ctx, state + STATE_TARGET_ON, 0);
        Log("target camera: off");
        return;
    }
    wr8(ctx, state + STATE_TARGET, static_cast<u32>(next));
    wr8(ctx, state + STATE_TARGET_ON, 1);
    Log(fmt::format("target camera: the monster of slot {}", next));
}

// ── (research, mh3u_aim_trace) what turns as the hunter aims: its action, the facing's u16s round unit + 0xed8, the
// state block's angles at + 0x110.. (0x009181bc builds the aiming camera from + 0x114, + 0x118, + 0x11c), the camera's
// mode (sGameCamera + 0x20), the weapon out, the pad: a line when one of them changes
void AimTrace(RCtx* ctx) {
    static u32 last[12] = {};
    static u32 frame = 0;
    ++frame;
    const u32 hunter = LocalHunter(ctx);
    if (!hunter) return;
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const u32 gamecam = rd32(ctx, 0x00c3b9cc);
    const u32 pad = rd32(ctx, SAPPPAD);
    const u32 now[12] = {rd8(ctx, state + STATE_WEAPON) << 24 | rd8(ctx, state + STATE_MODE) << 16 | rd16(ctx, state + STATE_ACTION),
                         rd16(ctx, hunter + 0xed4), rd16(ctx, hunter + 0xed6), rd16(ctx, hunter + 0xed8),
                         rd16(ctx, hunter + 0xeda), rd32(ctx, state + 0x110), rd32(ctx, state + 0x114),
                         rd32(ctx, state + 0x118), rd32(ctx, state + 0x11c),
                         HeapPointer(gamecam) ? rd8(ctx, gamecam + 0x20) : 0xffu,
                         rd8(ctx, state + 0x58) | rd8(ctx, state + 0xe77) << 8,
                         HeapPointer(pad) ? rd32(ctx, pad + PAD_HELD) : 0u};
    if (std::equal(std::begin(now), std::end(now), std::begin(last))) return;
    std::copy(std::begin(now), std::end(now), std::begin(last));
    Log(fmt::format("aim trace: f{} weapon {} mode {} action {:04x} | unit ed4 {:04x} ed6 {:04x} ed8 {:04x} eda {:04x} | "
                    "state 110 {:08x} 114 {:08x} 118 {:08x} 11c {:08x} | camera {} out {} height {} held {:08x}",
                    frame, now[0] >> 24, (now[0] >> 16) & 0xff, now[0] & 0xffff, now[1], now[2], now[3], now[4], now[5],
                    now[6], now[7], now[8], now[9], now[10] & 0xff, static_cast<s8>(now[10] >> 8), now[11]));
}

// (research, mh3u_eat_trace) a drink or a meal (mode 0, 0x35-0x38: 0x0092e54c): each frame its motion speed (the state
// block's + 0x600, f32: 1.8 with the Speed Eating skills 0xb1 / 0xb2 in the frames 0x008eb3e4 lets through), and once
// the skills it has (+ 0xdfc, ten u16s) and sGame + 0x28 / + 0x2b
void EatTrace(RCtx* ctx) {
    static u32 frame = 0, last_action = 0;
    ++frame;
    const u32 hunter = LocalHunter(ctx);
    if (!hunter) return;
    const u32 state = rd32(ctx, hunter + HUNTER_STATE);
    const u32 action = rd16(ctx, state + STATE_ACTION);
    if (rd8(ctx, state + STATE_MODE) != 0 || action < 0x35 || action > 0x38) {
        last_action = 0;
        return;
    }
    if (action != last_action) {
        std::string skills;
        for (u32 i = 0; i < 10; ++i) skills += fmt::format(" {:x}", rd16(ctx, state + 0xdfc + 2 * i));
        const u32 game = rd32(ctx, SGAME);
        Log(fmt::format("eat trace: f{} action {:02x}, skills{}, sGame + 0x28 {} + 0x2b {}", frame, action, skills,
                        HeapPointer(game) ? rd8(ctx, game + 0x28) : 0xffu, HeapPointer(game) ? rd8(ctx, game + 0x2b) : 0xffu));
        last_action = action;
    }
    Log(fmt::format("eat trace: f{} action {:02x} speed {:.2f}", frame, action, ReadF32(ctx, state + 0x600)));
}

// ── The end of the wait after a quest (skip_end) ──

double end_hold_since = 0;
bool end_requested = false, end_cut = false;
u32 quest_seen = 0;  // the quest block whose wait ran (a new quest: everything again)


void EndWaitStep(RCtx* ctx) {
    const u32 quest = rd32(ctx, SQUEST);
    const u32 block = HeapPointer(quest) ? rd32(ctx, quest + SQUEST_BLOCK) : 0;
    const u32 game = rd32(ctx, SGAME);
    if (options.test_complete && HeapPointer(block) && static_cast<s8>(rd8(ctx, block + Q_STATE)) == 1) {
        // (tests: what the quest's update calls when its objectives are met offline, 0x00633178)
        options.test_complete = false;
        CallGame(ctx, FN_QUEST_COMPLETE, {quest});
        Log("mh3u test: the quest completed by its own function");
    }
    if (!HeapPointer(block) || !HeapPointer(game)) {
        Hooks::Mh3uEndSkip(false, 0.0f, false);
        return;
    }
    const s32 left = static_cast<s32>(rd32(ctx, block + Q_WAIT));
    const bool offered = static_cast<s8>(rd8(ctx, block + Q_STATE)) == 2 && rd8(ctx, block + Q_COUNTING) != 0 &&
                         rd8(ctx, game + SGAME_RESULT) == 3 && left > 0;
    if (!offered) {
        if (quest_seen && static_cast<s8>(rd8(ctx, block + Q_STATE)) != 2) {  // (the wait is over: the next quest anew)
            quest_seen = 0, end_requested = end_cut = false, end_hold_since = 0;
            if (platform) platform->VoteEndSkip(false);
        }
        Hooks::Mh3uEndSkip(false, end_requested ? 1.0f : 0.0f, end_requested);
        return;
    }
    if (!quest_seen) {
        quest_seen = block;
        if (options.trace) Log(fmt::format("mh3u: the quest is complete, the wait before the return is {} (1/60 s)", left));
    }
    float hold = end_requested ? 1.0f : 0.0f;
    const u64 combo = options.skip_end;
    if (combo && !end_requested && (HeldButtons(ctx) & combo) == combo) {
        const double now = GameSeconds();
        if (end_hold_since == 0) end_hold_since = now;
        hold = std::min(1.0f, static_cast<float>((now - end_hold_since) / options.skip_end_hold));
        if (hold >= 1.0f) {
            end_requested = true;
            if (platform) platform->VoteEndSkip(true);
            Log("skip: the buttons held: asking to end the wait");
        }
    } else if (!end_requested) {
        end_hold_since = 0;
    }
    Hooks::Mh3uEndSkip(true, hold, end_requested);
    if (end_requested && !end_cut && (!platform || platform->EndSkipAgreed())) {
        wr32(ctx, block + Q_WAIT, 0);
        const double zero = 0.0;
        u32 words[2];
        std::memcpy(words, &zero, 8);
        wr32(ctx, block + Q_WAIT_REAL, words[0]);
        wr32(ctx, block + Q_WAIT_REAL + 4, words[1]);
        end_cut = true;
        Log(fmt::format("skip: everyone asked; the wait ended with {} units left", left));
    }
}

// ── The rewards screen: no time limit, and Send to Item Box sends every reward ──

float rewards_held_at = -1.0f;  // the countdown's value it is held at (-1: not holding)
u32 rewards_sent_list = 0;      // the list Send to Item Box ran on (0: none since it was left)

void RewardsStep(RCtx* ctx) {
    const u32 result = rd32(ctx, SRESULT);
    const u32 page = HeapPointer(result) ? result + RESULT_PAGE : 0;
    const bool running = page && rd8(ctx, page + D_RUNNING) == 1 && rd8(ctx, page + D_RUNNING2) == 1;
    const u32 number = running ? rd32(ctx, page + D_PAGE) : 0;
    if (!running || number < 1 || number > 3) {
        rewards_held_at = -1.0f, rewards_sent_list = 0;
        return;
    }
    if (options.rewards_no_limit) {
        const float left = ReadF32(ctx, page + D_LEFT);
        if (std::isfinite(left) && left > 0) {
            if (rewards_held_at < 0 || left > rewards_held_at + 60.0f) {  // (a new page: hold it at the middle of a second)
                rewards_held_at = left >= 90.0f ? left - std::fmod(left, 60.0f) - 30.0f : left;
                if (options.trace) Log(fmt::format("qol: the rewards page's countdown held at {:.0f}", rewards_held_at));
            }
            WriteF32(ctx, page + D_LEFT, rewards_held_at);
        }
    } else {
        rewards_held_at = -1.0f;
    }
    const bool sending = (number == 1 || number == 2) && rd8(ctx, page + D_SUB) == 1 && rd8(ctx, page + D_OPTION) == 1;
    const u32 list = page + (number == 1 ? D_LIST1 : D_LIST2);
    if (!sending) {
        rewards_sent_list = 0;
        return;
    }
    if (!options.rewards_send_all || rewards_sent_list == list) return;
    rewards_sent_list = list;
    const u32 user = rd32(ctx, SUSERDATA);
    if (!HeapPointer(user)) return;
    // (the box's add takes an out* in its caller's frame: a block below the guest's stack)
    const u32 sp = ctx->r[13];
    const u32 out = (sp - 0x200) & ~7u;
    ctx->r[13] = out;
    int sent = 0, kept = 0, special = 0;
    for (u32 i = 0; i < D_SLOTS; ++i) {
        const u32 slot = list + 4 * i;
        const u32 item = rd16(ctx, slot);
        const s16 count = static_cast<s16>(rd16(ctx, slot + 2));
        if (item == 0 || count <= 0) continue;
        const u32 data = CallGame(ctx, FN_ITEM_DATA, {item});
        if (data >= 0x00100000 && (rd8(ctx, data + 2) & 0x10)) {  // (the screen's own items: left to its own step)
            ++special;
            continue;
        }
        const u32 room = CallGame(ctx, FN_BOX_ROOM, {user, user + 0x18});
        s32 fits = static_cast<s32>(CallGame(ctx, FN_BOX_FITS, {user, item, user + 0x1a0, room}));
        if (fits <= 0) {
            ++kept;
            continue;
        }
        fits = std::min<s32>(fits, count);
        CallGame(ctx, FN_BOX_ADD, {user, item, static_cast<u32>(fits), out});
        const s16 left = static_cast<s16>(count - fits);
        wr16(ctx, slot + 2, static_cast<u16>(left));
        if (left <= 0) wr16(ctx, slot, 0), wr16(ctx, slot + 2, 0);
        else ++kept;
        ++sent;
    }
    if (sent > 0) CallGame(ctx, FN_SOUND, {out + 0x100, SOUND_SENT});
    ctx->r[13] = sp;
    bool empty = true;  // (emptied: back to the menu, on Sell Items & Exit, as after the game's last A)
    for (u32 i = 0; i < D_SLOTS && empty; ++i) empty = rd16(ctx, list + 4 * i) == 0;
    if (empty) {
        wr8(ctx, page + D_SUB, 0);
        wr8(ctx, page + D_CURSOR, rd8(ctx, page + D_OPTIONS) - 1);
    }
    Log(fmt::format("qol: Send to Item Box sent {} rewards at once, {} left in the list{}", sent, kept,
                    special ? fmt::format(" ({} the screen keeps)", special) : std::string()));
}

// ── Where the player is, for the PC app's Discord Rich Presence (Presence::Info, game 3) ──

/// The place of the area on top of sArea's stack that is one of the known ones (the last first), else PLACE_NONE.
[[maybe_unused]] Place CurrentPlace(RCtx* ctx) {
    const u32 area = rd32(ctx, SAREA);
    if (!HeapPointer(area)) return PLACE_NONE;
    const u32 count = std::min<u32>(rd32(ctx, area + AREA_COUNT), AREA_SLOTS);
    for (u32 i = count; i-- > 0;) {
        const u32 a = rd32(ctx, area + AREA_STACK + 4 * i);
        if (!HeapPointer(a)) continue;
        const u32 vtable = rd32(ctx, a);
        for (const AreaClass& c : AREA_CLASSES) {
            if (c.vtable == vtable) return c.place;
        }
    }
    return PLACE_NONE;
}

/// The quest's map id (the QuestMap texts' row) of a stage: the day and night maps of the first six (1-6, 15-20) and
/// the Misty Peaks (12, 24: map 13); 0 for another (the arenas and the special maps: no name without the quest's own).
[[maybe_unused]] int MapOfStage(int stage) {
    if (stage >= 1 && stage <= 6) return stage;
    if (stage >= 15 && stage <= 20) return stage - 14;
    if (stage == 12 || stage == 24) return 13;
    return 0;
}

#ifndef __ANDROID__
u32 presence_frames = 0;

void PresenceStep(RCtx* ctx) {
    if (!Presence::Wanted() || presence_frames++ % 15 != 0) return;
    Presence::Info info;
    info.game = 3;
    const u32 gui = rd32(ctx, SGUI);
    if (HeapPointer(gui)) {
        const s8 language = static_cast<s8>(rd8(ctx, gui + GUI_LANGUAGE));
        info.language = language >= 1 && language <= 5 ? language : -1;
    }
    const Place place = CurrentPlace(ctx);
    const u32 hunter = LocalHunter(ctx);
    info.menus = place == PLACE_MENUS || (place == PLACE_NONE && !hunter);
    info.hunter = hunter != 0 && !info.menus;
    info.quest = place == PLACE_QUEST;
    info.town = place == PLACE_VILLAGE ? 21 : place == PLACE_PORT ? 22 : place == PLACE_WOODS ? 25 : -1;
    const u32 user = rd32(ctx, SUSERDATA);
    if (info.hunter && HeapPointer(user)) {  // (the save's hunter: its name, UTF-8 at sUserData + 0x1b: the save's + 7)
        for (u32 i = 0; i < USER_NAME_BYTES; ++i) {
            const u32 c = rd8(ctx, user + USER_NAME + i);
            if (c == 0) break;
            if (c < 0x20) {
                info.name.clear();
                break;
            }
            info.name += static_cast<char>(c);
        }
    }
    if (info.hunter && HeapPointer(user)) {  // (the Hunter Rank: 0 before the Guild's registration, shown then without it)
        const u32 hr = rd16(ctx, user + USER_HR);
        info.hr = hr >= 1 && hr <= 999 ? static_cast<int>(hr) : 0;
    }
    if (info.hunter) {  // (the weapon out: the hunter's state block + 2, MH4U's weapon numbers, 0 the great sword .. 12)
        const u32 weapon = rd8(ctx, rd32(ctx, hunter + HUNTER_STATE) + STATE_WEAPON);
        info.weapon = weapon <= 12 && weapon != 5 ? static_cast<int>(weapon) : -1;
    }
    const u32 stage = rd32(ctx, SSTAGE);
    if ((info.quest || place == PLACE_WOODS) && HeapPointer(stage)) {
        info.stage = MapOfStage(rd8(ctx, stage + STAGE_NUMBER));
        info.area = rd8(ctx, stage + STAGE_AREA);
    }
    if (info.quest) {
        // (the quest's number, its quest/us/q_<n>.quest file's: sGame + 0x44, set as the quest is chosen and kept while it is
        // played; sQuest + 0xafbe has it too. Seen in the runs: 11106, The Fisherman's Tale)
        const u32 game = rd32(ctx, SGAME);
        const u32 id = HeapPointer(game) ? rd32(ctx, game + SGAME_QUEST_ID) : 0;
        info.quest_id = id > 0 && id < 100000 ? static_cast<int>(id) : 0;
        const u32 players = rd32(ctx, SPLAYER);
        for (u32 n = 0; n < PLAYER_COUNT && HeapPointer(players); ++n) {
            const u32 unit = rd32(ctx, players + SPLAYER_UNITS + 4 * n);
            const u32 kind = HeapPointer(unit) ? rd32(ctx, unit + UNIT_KIND) & 7 : 0;
            info.hunters += kind == 1 || kind == 2;
        }
        const u32 enemies = rd32(ctx, SENEMY);
        for (u32 i = 0; i < ENEMY_SLOT_COUNT && HeapPointer(enemies); ++i) {
            const u32 unit = Monster(ctx, enemies, i);
            if (!unit || !LargeMonster(ctx, unit) || !Targetable(ctx, unit)) continue;
            const int id = static_cast<int>(rd8(ctx, unit + MONSTER_KIND));
            if (id > 0 && std::find(info.monsters.begin(), info.monsters.end(), id) == info.monsters.end())
                info.monsters.push_back(id);
        }
    }
    if (options.trace) {
        static int last_place = -1, last_stage = -2, last_area = -2;
        if (place != last_place || info.stage != last_stage || info.area != last_area) {
            Log(fmt::format("mh3u presence: place {} town {} quest {} stage {} map {} area {} hunters {} language {}",
                            static_cast<int>(place), info.town, info.quest, HeapPointer(stage) ? rd8(ctx, stage + STAGE_NUMBER) : -1,
                            info.stage, info.area, info.hunters, info.language));
            if (place == PLACE_QUEST && last_place != PLACE_QUEST) {  // (research: where the quest's number is)
                const u32 game = rd32(ctx, SGAME), quest = rd32(ctx, SQUEST);
                auto words = [&](u32 at, u32 n) {
                    std::string out;
                    for (u32 i = 0; i < n; i += 4) out += fmt::format("{}{:08x}", i % 32 ? " " : fmt::format("\n  +{:x}: ", i), rd32(ctx, at + i));
                    return out;
                };
                if (HeapPointer(game)) Log("mh3u presence: sGame" + words(game, 0x398));
                if (HeapPointer(quest)) Log("mh3u presence: sQuest +0xaf80" + words(quest + 0xaf80, 0x80));
                const u32 block = HeapPointer(quest) ? rd32(ctx, quest + SQUEST_BLOCK) : 0;
                if (HeapPointer(block)) Log("mh3u presence: the quest block" + words(block, 0x200));
            }
            last_place = place, last_stage = info.stage, last_area = info.area;
        }
    }
    Presence::Publish(std::move(info));
}
#endif

}  // namespace

void Reset() {
    options = {};
    aim = {};
    aim_logged = 0, aim_height_rest = 0.0f;
    active = false;
    rest_yaw = rest_pitch = 0.0f;
    land = {};  // (a new game: its table read again)
    lock_hand_at = 0, lock_monster = 0, lock_time = 0;
    end_hold_since = 0, end_requested = end_cut = false, quest_seen = 0;
    rewards_held_at = -1.0f, rewards_sent_list = 0;
    hold_gathering = false, points_valid = false, points_hunter = 0, pouch_seen = 0;
    target_held = 0;
}

bool SetOption(const std::string& key, const std::string& value) {
    if (key == "qol_hold_gather") options.hold_gather = IsOn(value);
    else if (key == "qol_flex_cancel") options.flex_cancel = IsOn(value);
    else if (key == "qol_sharpen_cancel") options.sharpen_cancel = IsOn(value);
    else if (key == "qol_rewards_no_limit") options.rewards_no_limit = IsOn(value);
    else if (key == "rewards_send_all") options.rewards_send_all = IsOn(value);
    else if (key == "gather_all") options.gather_all = IsOn(value);
    else if (key == "skip_end") options.skip_end = PadBits(value);
    else if (key == "target") options.target = PadBits(value) & (PAD_ZL | PAD_ZR);
    else if (key == "skip_end_hold") options.skip_end_hold = std::clamp(std::strtof(value.c_str(), nullptr), 0.2f, 10.0f);
    else if (key == "mh3u_trace") return options.trace = IsOn(value), true;
    else if (key == "mh3u_camera_trace") return options.camera_trace = IsOn(value), true;
    else if (key == "mh3u_test_complete") return options.test_complete = IsOn(value), true;
    else if (key == "mh3u_lock_any") return options.lock_any = IsOn(value), true;
    else if (key == "mh3u_gather_trace") return options.gather_trace = IsOn(value), true;
    else if (key == "mh3u_aim_trace") return options.aim_trace = IsOn(value), true;
    else if (key == "mh3u_eat_trace") return options.eat_trace = IsOn(value), true;
    else if (key == "mh3u_aim_sign") {
        char* rest = nullptr;
        aim.yaw_sign = std::strtof(value.c_str(), &rest);
        aim.pitch_sign = rest && *rest == ',' ? std::strtof(rest + 1, nullptr) : aim.pitch_sign;
        return true;
    } else if (key == "mh3u_aim_log") return aim.trace = IsOn(value), true;
    return false;
}

void Install(const RCodeInfo& info, std::vector<std::string>& log, Platform* p, void (*log_line)(const std::string&)) {
    platform = p;
    log_fn = log_line;
    active = false;
    struct Point {
        u32 addr;
        const char* name;
        RHookFn fn;
    };
    static constexpr Point points[] = {
        {HOOK_CAMERA_TURN, "uGameCamera::turnByInput", CameraTurn},
        {HOOK_PLAYER_INPUT, "uPlayer::updateInput", PlayerInput},
    };
    int set = 0;
    for (const Point& point : points) {
        u32 addr = point.addr;
        for (u32 i = 0; info.hook_names && i < info.hook_count; ++i) {
            if (info.hook_names[i] && std::string_view(info.hook_names[i]) == point.name) addr = info.hook_addrs[i];
        }
        if (info.set_hook && info.set_hook(addr, point.fn)) ++set;
        else log.push_back(fmt::format("MH3U: no hook point for {} at {:08X} (rebuild the code library)", point.name, addr));
    }
    active = true;
    log.push_back(fmt::format("MH3U gameplay: {} of {} hook points (the camera's turn: the mouse and the lock-on), the end of "
                              "the wait on {}, rewards: no limit {}, send all {}",
                              set, std::size(points), options.skip_end ? "its buttons" : "nothing",
                              options.rewards_no_limit ? "on" : "off", options.rewards_send_all ? "on" : "off"));
}

bool Active() {
    return active;
}

void Frame(RCtx* ctx) {
    if (!active) return;
    AimStep(ctx);  // (before the hunter's step: its aim's turn)
    EndWaitStep(ctx);
    RewardsStep(ctx);
    GatherStep(ctx);
    TargetStep(ctx);
    if (options.aim_trace) AimTrace(ctx);
    if (options.eat_trace) EatTrace(ctx);
#ifndef __ANDROID__
    PresenceStep(ctx);
#endif
}

bool Supports(const std::string& key) {
    if (!active) return true;
    // (what MH3U doesn't have, or this port doesn't do on it yet; mh3u_qol.h)
    static constexpr std::string_view off[] = {
        "supply_box_wait",  // the supply box: MH3U takes an item at each press, with no wait between items to take away
        "rewards_palico",   // the rewards screen has no Palico's lists in MH3U
        "kick",             // the port's kick button: MH4U's touch-panel kick; MH3U has no such panel action
        "skip",             // the touch screen's Skip after a quest: MH3U has none (the end of the wait, skip_end, does it)
        // (Shown since mh3u-final, seen on the phones in the Moga Woods' area 1: gather_all, one press spent each point
        // with the point's own items in the pouch; qol_hold_gather, A held gathered a point again and again, 3 then 2
        // Herbs. Shown since mh3u-pcfinal, seen in MH3U.exe after The Fisherman's Tale and since mh3u-faltan on the
        // Moto after Harvest Tour: Deserted Isle: rewards_send_all, Send to Item Box sent the rewards at once, none
        // left in the list. Shown since mh3u-faltan, seen on the Moto in the Moga Woods: qol_flex_cancel and
        // qol_sharpen_cancel (CancelStep), gyro_aim (AimStep: the turn goes to the aiming hunter))
    };
    return std::find(std::begin(off), std::end(off), key) == std::end(off);
}

}  // namespace Port::Mh3uQol
