// MH3U's addresses and fields for the hunting aids (hunt_aids.cpp; docs/research/hunt-aids-mh3u.md), USA v1.0.
// MH3U's own: no MH4U address has an equivalent there, so they are plain numbers, never GameAddr (an .inc: the port's
// address tests, tests/test_port_addresses.py, are about MH4U's addresses in port/src's .cpp and .h).
#pragma once

#include <cstdint>

namespace Port::HuntAids::mh3u {

constexpr std::uint32_t PART_DAMAGE = 0x00838a94;  // uEnemy::addPartDamage: the hook point of By hit (config/hooks.txt)
constexpr std::uint32_t CODE_END = 0x00b00000;      // the end of .text (a getDTI is there)
constexpr std::uint32_t SENEMY = 0x00c1bff8;       // sEnemy*: 32 monster slots at +0x14, a slot good while [unit+4]&7 is 1 or 2
constexpr std::uint32_t ENEMY_SLOTS = 0x14;        //   (0x005bd89c)
constexpr std::uint32_t EM_STATUS = 0x1a48;        // monster: its status block (the unit's own +0xd60), as 0x008394bc reads it
constexpr std::uint32_t ST_KIND = 0x03;            // u8: its kind, the row of the game's monster names (GUI\font\Monster_<lang>)
constexpr std::uint32_t ST_FLAGS = 0x8c;           // u32: bit 0 a large monster (the Target Cam's list 0x0039d278, 0x0082bf8c)
constexpr std::uint32_t ST_AREA = 0xdb;            // u8: the area of the map it is in (0x0082bf8c against the stage's)
constexpr std::uint32_t ST_PAINT = 0x884;          // f32: the paint's time left, the monster on both maps while above 0: the
                                                   //   maps' signs (uIDCockpitSign's step, 0x0034c08c) ask 0x0082b1d4, which
                                                   //   reads it; a paintball's hit sets 18000 (0x0082cd34, from the hit step
                                                   //   0x0098473c, the hit's flags + 0x4e & 0x100), the monster's step takes
                                                   //   the frame's time off it (0x008307d8). (+ 0xa34, which 0x0084fe88 sets
                                                   //   to 7200 and 0x008338d0 reads, is another mark: no map reads it, so a
                                                   //   timer kept there showed nothing, mh3u-faltan's c_n7_map)
constexpr std::uint32_t ST_HEALTH = 0x7f8;         // s32: health; + 4 its maximum (the setter 0x008394bc clamps it)
constexpr std::uint32_t SSTAGE = 0x00c42554;       // sStage*: +0xee the hunter's area of the map (0x0082bf8c)
constexpr std::uint32_t STAGE_AREA = 0xee;
constexpr std::uint32_t SAREA = 0x00ca8910;        // sArea*: +0x3818 the areas running, +0x3820 a table of 8 cArea*
constexpr std::uint32_t SAREA_COUNT = 0x3818, SAREA_TABLE = 0x3820;
// the hunting grounds: a quest's areas (aQuest, aQuestArea) and the Moga Woods' free hunt (aFreeQuest,
// aFreeQuestArea), told by their class (MtDTI: config/mt_classes.tsv), which each area's getDTI (vtable + 0x14,
// "ldr r0, =DTI; bx lr") gives
constexpr std::uint32_t DTI_AQUEST = 0x00cb9598, DTI_AQUEST_AREA = 0x00cb95b8, DTI_AFREEQUEST = 0x00cb9438,
              DTI_AFREEQUEST_AREA = 0x00cb9458;
constexpr std::uint32_t SGAME = 0x00c3b978;        // sGame*: +0x2b (s8) this console's player number
constexpr std::uint32_t SGAME_PLAYER = 0x2b;
constexpr std::uint32_t HUNTER_STATE = 0xd5c;      // a hunter's unit: its state block*
constexpr std::uint32_t SPLAYER = 0x00c40738;      // sPlayer*: the hunters' units, 8 slots at +0x20 (0x008959c0)
constexpr std::uint32_t SPLAYER_UNITS = 0x20;
constexpr std::uint32_t SGUI = 0x00ca8924;         // sGUI*: +0x1c s8 the game's language, the row of its suffixes (0x00540e24)
constexpr std::uint32_t GUI_LANGUAGE = 0x1c;
constexpr std::uint32_t SCAMERA = 0x00ca8914;      // sCamera*: 8 viewports at +0x28, 0x198 bytes each: camera +0x04, display +0x15
constexpr std::uint32_t SCAMERA_VIEWPORTS = 0x28, VIEWPORT_SIZE = 0x198, VIEWPORT_CAMERA = 0x04, VIEWPORT_DISPLAY = 0x15;
constexpr std::uint32_t CAMERA_PROJ = 0x158;       // the camera's projection as its getProjMatrix (0x003320fc) copies it
constexpr std::uint32_t FRAME_COUNTER = 0x00ca8804;
constexpr std::uint32_t UNIT_CENTER = 0x170;       // vec3: the middle of the unit's bounding sphere (+ 0x17c its radius; a hunter's is 100 above its feet,
                                                   //   +0x40; the same sphere with a wider radius at +0x160)
constexpr std::uint32_t UNIT_HIT = 0;              // (no field of the last hit's place: the hit step hands it, HIT_STEP_PLACE)
// The hit step (a hit on a monster, every kind of attacker: 0x00983ce4(?, ?, hit, collision, ?, monster)) works out
// the hit's raw part in s16 (the attack x the hit zone x sharpness x the weapon's class factor) and the element's in s17
// (each element's attack x its zone, x the element's sharpness), and hands their sum to 0x00838a94(monster, part, total,
// ?), which counts it on the part, once (its only caller, at 0x00984b68). At that call the step's r4 is the hit: +0x0e
// how well it landed, as the game shows it (0x009840e4: the zone x sharpness 1.0 or more 4, 0.45 or more 3, 0.25 or
// more 2, else 0, a bounce; 6 a special one), +0x48 0x1f fixed damage (its +0x44 negated), +0x50 the elements (0x10 fire,
// 0x20 water, 0x200 thunder, 0x40 ice, 0x80 dragon, as MHGU's); s16 and s17 are callee-saved, so the hook reads them
// as the step left them.
constexpr std::uint32_t HIT_STEP_RETURN = 0x00984b6c;  // (the part damage's return address in the hit step)
constexpr std::uint32_t HIT_LEVEL = 0x0e, HIT_FIXED = 0x48, HIT_ELEMENTS = 0x50;
// u16: the hit's marks; the hunter's attack (0x009009ec, as it rolls the affinity: the save's roll +0x190 % 100 against
// the affinity 0x00917f90 gives) sets 0x200 on a critical hit (x1.25) and 0x100 on a feeble one (x0.75); the
// gunners' shells the same (0x009d90c8)
constexpr std::uint32_t HIT_MARKS = 0x5a;
constexpr std::uint32_t HIT_STEP_PLACE = 0xbc;  // (the step's frame: its last argument, a vec3* where the hit landed)
// (research) the player's equipment as the game keeps it (sUserData, weapon_tree_mh3u.inc's): the weapon worn + 0x40,
// the box from + 0x1140, 16 bytes a piece (the kind + 0: 7 + the weapon's class, the id u16 + 2)
constexpr std::uint32_t SUSERDATA = 0x00c43360, USER_WORN = 0x40, USER_BOX = 0x1140, BOX_SLOT = 16, BOX_SLOTS = 1000;
// (research) the hunter's profile is copied (name, zenny, the equipment worn) into sFreeQuest [0x00c3a740] (+ 0xb0c the
// weapon: the Moga Woods' hunter is made from it) and sQuest [0x00c409d0] (+ 0xd74): live1's dumps
constexpr std::uint32_t SFREEQUEST = 0x00c3a740, FREEQUEST_WORN = 0xb0c, SQUEST = 0x00c409d0, QUEST_WORN = 0xd74;
// (research) u16s of the state block (unit + 0xd60) the trace shows: + 0x90 and + 0xa8 (not angles), + 0x118 a copy of
// the facing (mh3u-qol's "facing": spun, the hunter doesn't turn)
constexpr std::uint32_t STATE_FACING = 0x90, STATE_FACING2 = 0xa8, STATE_FACING3 = 0x118;
// The hunter's facing (u16, 0x10000 a turn; the yaw of its unit's quaternion + 0x50: 2 atan2(y, w)): the state block's
// + 0x178 (unit + 0xed8) is the one the game takes (turned a little each frame on the Anbernic, the hunter turns round;
// + 0x118, unit + 0xc8c, + 0xf1c and + 0xf1e hold copies the game makes from it, and spun alone turn nothing). Toward a
// point it is atan2(dx, dz), as MH4U's.
constexpr std::uint32_t UNIT_FACING = 0xed8;
constexpr std::uint32_t UNIT_FACING_COPIES[] = {0xc8c, 0xe78, 0xf1c, 0xf1e};
// The hunter: + 0x02 its weapon class (MH4U's numbering: 10 the bow), + 0x58 1 with the weapon out (mh3u-pantalla's),
// + 0x56 the area of the map it is in, + 0xd8 its place (x, y, z); + 0x14a9 1 while the Target Cam is on, + 0x14a8 the
// Target Cam's monster, its sEnemy slot (mh3u-qol's)
constexpr std::uint32_t ST_WEAPON = 0x02, ST_WEAPON_OUT = 0x58, ST_HUNTER_AREA = 0x56, ST_PLACE = 0xd8;
constexpr std::uint32_t ST_TARGET = 0x14a8, ST_TARGET_ON = 0x14a9;
constexpr std::uint8_t WEAPON_BOW = 10;
// sAppPad's buttons: X 0x100 (the bow's draw and shot), the Circle Pad's four ways 0x3c000000
constexpr std::uint32_t PAD_X = 0x100, PAD_STICK = 0x3c000000;
// (research) sAppPad*: +0xd4 the game's buttons held, +0xdc those pressed this frame (mh3u-qol's)
constexpr std::uint32_t SAPPPAD = 0x00c1bcb0, PAD_HELD = 0xd4;
constexpr float PAINT_KEEP = 1000.0f;    // the timer kept on a monster: under a paintball's 18000, so that turned off
constexpr float PAINT_LOW = 900.0f;      //   ours can be told from the player's paint

}  // namespace Port::HuntAids::mh3u
