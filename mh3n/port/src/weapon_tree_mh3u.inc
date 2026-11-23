// The weapon tree on MH3U (USA v1.0): its weapon tables (included by weapon_tree.cpp, inside its anonymous namespace).
// MH3U's own addresses, plain numbers (an .inc: tests/test_port_addresses.py is about MH4U's).
//
// MH3U numbers its weapon classes 0 the great sword, 1 sword and shield, 2 hammer, 3 lance, 4 heavy bowgun, (5 none),
// 6 light bowgun, 7 long sword, 8 switch axe, 9 gunlance, 10 bow, 11 dual blades, 12 hunting horn; the port's tree (and
// the app's panel) uses MH4U's equipment types, so each class is published as MH4U's (Mh4uType). Three tables a
// class, each a pointer table of 13 by class (found from sPlayer's constructor, 0x00895a08, which keeps the first at
// sPlayer + 0x94, and the tables beside it):
// - the stats [0x00ba353c + 4 * class]: a melee weapon 28 bytes an id: +0x00 u16 its model, +0x02 its rarity - 1, +0x04
//   u32 its price (upgrading into it costs it; forging it from scratch half as much again: the Smithy's lists, Iron
//   Sword 500 -> 750z, Cataclysm Sword 7500 -> 11250z, Iron Sword+'s upgrade 1500z), +0x08 its sharpness row and +0x09 its sharpness level (below),
//   +0x0a u16 the true attack (the game shows it times the class's multiplier), +0x0c the defense it adds (the
//   Barroth's and the Uragaan's: 20, 30, 40), +0x0d s8 its affinity (the Nargacuga's line 20 to 40), +0x0e its element
//   (1 fire, 2 water, 3 thunder, 4 dragon, 5 ice) and +0x0f s8 its value / 10, negative where it needs Awaken (the
//   hunter's elements, 0x00581460: the value is used negated only with that skill), +0x10 and +0x11 a second element
//   the same way (dual blades: Dichotomy's ice and fire), +0x12 its status (1 poison, 2 paralysis, 3 sleep, 4 blast:
//   the Rathian's poison, Plesioth Lullaby's sleep, the Brachydios's blast) and +0x13 its value the same way, +0x14
//   its slots (0 to 3, more up a line), +0x16 1 or 2 for a weapon of the game's (0 an unused id), +0x17 its place in
//   the lists. No weapon has both an element and a status;
// - the sharpness: the rows at 0x00ba3c50, 91 of 7 u32, where each colour ends (red, orange, yellow, green, blue,
//   white, purple; 450 the whole bar); a weapon has its row up to 150 + 50 x its level (the hunter's sharpness,
//   0x008d2f00: the row to the hunter's +0xaf2, its end 150 + 50 x the level to +0xaf6, 50 more with Handicraft, at
//   most 450; the status screen draws the row from the same table, 0x0034ebe0), as MH4U's tables do;
// - a ranged class's stats are 100 bytes an id (0x00895594): +0x02 its rarity - 1, +0x04 u16 the true attack, +0x06 the
//   defense it adds (Hunter's Bow I's +5 in the Smithy), +0x07 not 0 for a weapon of the game's, +0x0c u32 its price
//   (as a melee weapon's), +0x11 its slots, +0x12 s8 its affinity (the hunter's affinity,
//   0x0091b324: a melee weapon's +0x0d, a ranged one's +0x12), and a bow's element +0x14 and value / 10 +0x15 (s8,
//   negative where it needs Awaken, 0x005814b8; a bowgun's +0x14 is no element);
// - the upgrades [0x00c06bb4 + 4 * class] - 12: 28 bytes an id: u16 the id itself when it is upgraded from the id before
//   it (the line's next weapon; 0 when not), u16 x 4 the other weapons it upgrades into (its branches), then at +0x0c four
//   materials (u16 item, u16 count) upgrading into it takes;
// - the create recipes [0x00c06b64 + 4 * class] - 4: 24 bytes each, u16 x 3, the id u16 at +0x06, four materials from
//   +0x08; the first with id 0 after the first ends them.
// Checked against the game's own trees (Iron Sword -> Iron Sword+ with 5 Iron Ore -> Buster Sword ...; Hunter's Knife:
// Iron Ore x3 to forge; Peckish Peco: attack 120). Seen against the Smithy's own stats in MH3U.exe (mh3u-pc3): Cataclysm
// Sword's water 100, 1 slot and sharpness bar, Sponge Gear I's water 50, Hunter's Bow I's attack 96 and defense +5, the
// forge lists' prices; a hidden element (Iron Sword's ice 50, Bone Blade's poison 100) the Smithy doesn't show at all. A class's table ends where the next one in memory begins: the
// names' files have a few more ids than the great sword's table (136 of 138), so ids past a table's end are not read.
namespace mh3u_tables {

constexpr std::uint32_t STATS = 0x00ba353c, UPGRADES = 0x00c06bb4, CREATES = 0x00c06b64, SHARPNESS = 0x00ba3c50;
constexpr int kSharpnessRows = 91;
constexpr int kTypes = 13;
// each MH3U class's MH4U equipment type (-1 none), the count of ids (its names' file less one: MH3U's own lists), the
// attack's multiplier as the game shows it, and whether its stats are a melee weapon's 28 bytes
struct ClassInfo {
    int mh4u_type;
    int count;
    float multiplier;
    bool melee;
};
constexpr ClassInfo kInfo[kTypes] = {
    {7, 138, 4.8f, true},    // 0 great sword (Lsword)
    {8, 141, 1.4f, true},    // 1 sword and shield (Sword)
    {9, 135, 5.2f, true},    // 2 hammer
    {10, 147, 2.3f, true},   // 3 lance
    {12, 92, 1.5f, false},   // 4 heavy bowgun (Hbg)
    {-1, 0, 0.0f, false},    // 5 (none)
    {11, 96, 1.3f, false},   // 6 light bowgun (Lbg)
    {13, 117, 4.8f, true},   // 7 long sword (Lsword2)
    {14, 115, 5.4f, true},   // 8 switch axe (Axe)
    {15, 118, 2.3f, true},   // 9 gunlance
    {16, 116, 1.2f, false},  // 10 bow
    {17, 126, 1.4f, true},   // 11 dual blades (WSword)
    {18, 103, 5.2f, true},   // 12 hunting horn (Pipe)
};

/// MH3U's class -> the port's equipment type (7 ... 18), or -1.
int Mh4uType(int mh3u_class) {
    return mh3u_class >= 0 && mh3u_class < kTypes ? kInfo[mh3u_class].mh4u_type : -1;
}

/// The port's equipment type -> MH3U's class, or -1.
int Mh3uClass(int mh4u_type) {
    for (int t = 0; t < kTypes; ++t) {
        if (kInfo[t].mh4u_type == mh4u_type) return t;
    }
    return -1;
}

bool DataPointer(std::uint32_t p) {
    return p >= 0x00b00000 && p < 0x00cba000;
}

/// Every class's weapons from MH3U's tables, in the port's class order (kFirstType ...). False when a table isn't where
/// it is known to be (another version, a mod that moved it).
bool Read(RCtx* ctx, std::vector<weapon_tree::Class>* out, int* weapons) {
    out->assign(kClasses, {});
    *weapons = 0;
    std::uint32_t starts[kTypes] = {};  // (each class's stats table, for where each one ends)
    for (int t = 0; t < kTypes; ++t) {
        if (kInfo[t].mh4u_type >= 0) starts[t] = rd32(ctx, STATS + 4 * t);
    }
    for (int t = 0; t < kTypes; ++t) {
        const ClassInfo& info = kInfo[t];
        if (info.mh4u_type < 0) continue;
        const std::uint32_t stats = starts[t], upgrades = rd32(ctx, UPGRADES + 4 * t) - 12,
                            creates = rd32(ctx, CREATES + 4 * t) - 4;
        if (!DataPointer(stats) || !DataPointer(upgrades) || !DataPointer(creates)) return false;
        const std::uint32_t size = info.melee ? 28 : 100;
        int count = info.count;  // (the ids before the next table in memory)
        for (int o = 0; o < kTypes; ++o) {
            if (starts[o] > stats) count = std::min<int>(count, static_cast<int>((starts[o] - stats) / size) - 1);
        }
        std::vector<weapon_tree::Weapon>& ws = (*out)[info.mh4u_type - kFirstType].weapons;
        ws.assign(info.count + 1, {});
        for (int id = 1; id <= count; ++id) {
            weapon_tree::Weapon& w = ws[id];
            const std::uint32_t u = upgrades + 28 * id;
            int element = 0, element_value = 0, element2 = 0, element2_value = 0, status = 0, status_value = 0;
            if (info.melee) {
                const std::uint32_t r = stats + 28 * id;
                const int flag = rd8(ctx, r + 0x16);
                w.real = flag == 1 || flag == 2;
                w.rarity = rd8(ctx, r + 0x02) + 1;
                w.attack = static_cast<int>(std::lround(rd16(ctx, r + 0x0a) * info.multiplier));
                w.create.zenny = static_cast<int>(rd32(ctx, r + 0x04));
                w.defense = rd8(ctx, r + 0x0c);
                w.affinity = static_cast<signed char>(rd8(ctx, r + 0x0d));
                w.slots = std::min<int>(rd8(ctx, r + 0x14), 3);
                element = rd8(ctx, r + 0x0e), element_value = static_cast<signed char>(rd8(ctx, r + 0x0f));
                element2 = rd8(ctx, r + 0x10), element2_value = static_cast<signed char>(rd8(ctx, r + 0x11));
                status = rd8(ctx, r + 0x12), status_value = static_cast<signed char>(rd8(ctx, r + 0x13));
                const int row = rd8(ctx, r + 0x08), level = rd8(ctx, r + 0x09);
                if (row < kSharpnessRows && level <= 6) {  // (the row up to its level's cut)
                    for (int k = 0; k < 7; ++k) {
                        const int end = static_cast<int>(rd32(ctx, SHARPNESS + 28 * row + 4 * k));
                        w.sharpness.push_back(std::min(end, 150 + 50 * level));
                    }
                }
            } else {  // (a bow or a bowgun: 100 bytes an id)
                const std::uint32_t r = stats + 100 * id;
                w.real = rd8(ctx, r + 0x07) != 0;
                w.rarity = rd8(ctx, r + 0x02) + 1;
                w.attack = static_cast<int>(std::lround(rd16(ctx, r + 0x04) * info.multiplier));
                w.create.zenny = static_cast<int>(rd32(ctx, r + 0x0c));
                w.defense = rd8(ctx, r + 0x06);
                w.slots = std::min<int>(rd8(ctx, r + 0x11), 3);
                w.affinity = static_cast<signed char>(rd8(ctx, r + 0x12));
                if (info.mh4u_type == 16) {  // (a bow's element; a bowgun's ammo instead)
                    element = rd8(ctx, r + 0x14), element_value = static_cast<signed char>(rd8(ctx, r + 0x15));
                }
            }
            // its element (or status) first, then a second element or its status; hidden where it needs Awaken
            const bool has_element = ElementOf(element) != weapon_tree::kElementNone && element_value != 0;
            if (has_element) {
                w.element = ElementOf(element), w.element_value = std::abs(element_value) * 10;
                w.element_hidden = element_value < 0;
            }
            int second = 0, second_value = 0;
            if (ElementOf(element2) != weapon_tree::kElementNone && element2_value != 0) {
                second = ElementOf(element2), second_value = element2_value;
            } else if (StatusOf(status) != weapon_tree::kElementNone && status_value != 0) {
                second = StatusOf(status), second_value = status_value;
            }
            if (second != weapon_tree::kElementNone) {
                int& kind = has_element ? w.element2 : w.element;
                int& value = has_element ? w.element2_value : w.element_value;
                kind = second, value = std::abs(second_value) * 10;
                if (!has_element) w.element_hidden = second_value < 0;
            }
            const int price = w.create.zenny;
            // the line's next weapon (its upgrade record says it comes from this one), then its branches
            if (id < count && static_cast<int>(rd16(ctx, upgrades + 28 * (id + 1))) == id + 1) w.children.push_back(id + 1);
            for (int k = 0; k < 4; ++k) {
                const int child = static_cast<int>(rd16(ctx, u + 2 + 2 * k));
                if (child > 0 && child <= count && child != id + 1) w.children.push_back(child);
            }
            Materials(ctx, u + 0x0c, 4, &w.upgrade);
            if (!w.upgrade.items.empty()) w.upgrade.zenny = price;
        }
        for (int k = 0; k < 400; ++k) {  // the create recipes
            const std::uint32_t r = creates + 24 * k;
            const int id = static_cast<int>(rd16(ctx, r + 6));
            if (k > 0 && id == 0) break;
            if (id <= 0 || id > count) continue;
            weapon_tree::Weapon& w = ws[id];
            w.forge = true;
            if (w.create.items.empty()) Materials(ctx, r + 8, 4, &w.create);
        }
        for (weapon_tree::Weapon& w : ws) {
            if (!w.forge) w.create = weapon_tree::Recipe{};  // (its price only where a create recipe is)
            else w.create.zenny = w.create.zenny * 3 / 2;     // (forging from scratch: the price and half again)
            *weapons += w.real ? 1 : 0;
        }
    }
    return true;
}

// The player's equipment (sUserData [0x00c43360], the save as the game keeps it): the box from + 0x1140, 16 bytes a
// slot {u8 kind, u8 0, u16 id, ...}, a weapon's kind 7 + its MH3U class (7 the great sword ... 19 the hunting horn;
// the armour 1 to 5); the equipment worn from + 0x40, the weapon first, as the box's slots. (Seen in a new hunter's
// save: one weapon of each class in the box, Hunter's Bow I among them, the Hunter's Knife worn.)
constexpr std::uint32_t SUSERDATA = 0x00c43360, BOX = 0x1140, WORN = 0x40, SLOT = 16;
constexpr int kBoxSlots = 1000;

bool HeapPointer(std::uint32_t p) {
    return (p & 3) == 0 && ((p >= 0x08000000 && p < 0x10000000) || (p >= 0x14000000 && p < 0x1c000000));
}

/// The box's weapons (by the port's class, by id: 1 owned) and the weapon worn (the port's type and its id; -1 none).
bool ReadBox(RCtx* ctx, std::vector<std::vector<std::uint8_t>>* had, int* worn_type, int* worn_id) {
    const std::uint32_t user = rd32(ctx, SUSERDATA);
    if (!HeapPointer(user)) return false;
    had->assign(kClasses, {});
    for (int t = 0; t < kTypes; ++t) {
        if (kInfo[t].mh4u_type >= 0) (*had)[kInfo[t].mh4u_type - kFirstType].assign(kInfo[t].count + 1, 0);
    }
    for (int i = 0; i < kBoxSlots; ++i) {
        const std::uint32_t slot = user + BOX + SLOT * i;
        const int t = Mh4uType(static_cast<int>(rd8(ctx, slot)) - 7);
        const int id = static_cast<int>(rd16(ctx, slot + 2));
        if (t < 0) continue;
        std::vector<std::uint8_t>& list = (*had)[t - kFirstType];
        if (id > 0 && id < static_cast<int>(list.size())) list[id] = 1;
    }
    *worn_type = Mh4uType(static_cast<int>(rd8(ctx, user + WORN)) - 7);
    *worn_id = *worn_type >= 0 ? static_cast<int>(rd16(ctx, user + WORN + 2)) : -1;
    return true;
}

// The Smithy (found from its forge list's builder, 0x006051a8, and the towns' facility switch, 0x005e7e80): the town's
// controller sLobby [0x00c406c0] runs one facility at a time, its number the byte + 0x4d18 (15 the Smithy, whose step
// is 0x005cc554); the Smithy keeps its state at + 0x5690, as MH4U's does: + 0x00 its state (5 the forge's list ...),
// + 0x07 the equipment kind listed, + 0x10 the list's row under the cursor (s16), + 0x1c the list's rows, and from
// + 0x3e8 the rows, 24 bytes each: the equipment kind + 0 (7 + the weapon's class), its id u16 + 4. Those rows are the
// forge list's (state 5, its step 0x005d9f70): Upgrade's list (state 9, 0x005d4264) keeps its own, a grid of the box
// (it leaves these rows as the last forge list had them, as do the menus around it, so they are read only in state 5).
// Upgrade's grid is at the town's controller + 0x5bb8: + 0x02 the box's page (u8), + 0x0c the column and + 0x0e the row
// under the cursor (u16); the slot is page * 100 + row * 10 + column (0x0060bf18, which Upgrade's A takes into the box:
// sUserData + 0x1140 + 16 * slot, its equipment kind + 0 and id + 2, as ReadBox's), kept while its upgrade screen shows.
// That screen (the state's sub-state + 0x01 not 0) lists what the weapon upgrades into (0x00616870 builds it): at the
// Smithy's + 0x27c, 16 bytes each as the box's slots, + 0x166 how many (u16, at most 6), the row under the cursor
// + 0x164 plus 6 x + 0x16 (s16 each, 0x005d4264's own reading): there the tree is the one under that cursor, as MH4U's
// Upgrade list shows the weapons it upgrades into.
constexpr std::uint32_t SLOBBY = 0x00c406c0, FACILITY = 0x4d18, SMITHY_BLOCK = 0x5690;
constexpr std::uint8_t FACILITY_SMITHY = 15;
constexpr std::uint8_t S_FORGE_LIST = 5, S_UPGRADE_LIST = 9;
constexpr std::uint32_t S_STATE = 0x00, S_KIND = 0x07, S_ROW = 0x10, S_ROWS = 0x1c, S_LIST = 0x3e8, S_ROW_SIZE = 24;
constexpr std::uint32_t UPGRADE_GRID = 0x5bb8, G_PAGE = 0x02, G_COLUMN = 0x0c, G_ROW = 0x0e;
constexpr int kGridSide = 10, kGridPage = kGridSide * kGridSide;
constexpr std::uint32_t S_SUB = 0x01, S_INTO = 0x27c, S_INTO_COUNT = 0x166, S_INTO_ROW = 0x164, S_INTO_PAGE = 0x16;
constexpr int kIntoMax = 6;
constexpr int kMaxSmithyRows = 600;
constexpr std::uint32_t SGUI = 0x00ca8924, GUI_LANGUAGE = 0x1c;  // the game's language (hunt_aids.cpp's)

struct SmithyNow {
    bool open = false;         // the Smithy runs
    int state = -1, sub = -1, rows = 0, row = -1;
    int type = -1, id = -1;    // the weapon under the cursor, as the port's type (-1 none)
    int language = -1;
};

SmithyNow ReadSmithy(RCtx* ctx) {
    SmithyNow n;
    const std::uint32_t gui = rd32(ctx, SGUI);
    if (HeapPointer(gui)) n.language = static_cast<std::int8_t>(rd8(ctx, gui + GUI_LANGUAGE));
    const std::uint32_t lobby = rd32(ctx, SLOBBY);
    if (!HeapPointer(lobby) || rd8(ctx, lobby + FACILITY) != FACILITY_SMITHY) return n;
    n.open = true;
    const std::uint32_t st = lobby + SMITHY_BLOCK;
    n.state = rd8(ctx, st + S_STATE);
    n.sub = rd8(ctx, st + S_SUB);
    n.rows = static_cast<std::int16_t>(rd16(ctx, st + S_ROWS));
    n.row = static_cast<std::int16_t>(rd16(ctx, st + S_ROW));
    if (n.state == S_FORGE_LIST && n.rows > 0 && n.rows <= kMaxSmithyRows && n.row >= 0 && n.row < n.rows) {
        const std::uint32_t r = st + S_LIST + S_ROW_SIZE * n.row;
        const int t = Mh4uType(static_cast<int>(rd8(ctx, r)) - 7);
        const int id = static_cast<int>(rd16(ctx, r + 4));
        const int c = Mh3uClass(t);
        if (t >= 0 && c >= 0 && id > 0 && id <= kInfo[c].count) n.type = t, n.id = id;
    } else if (n.state == S_UPGRADE_LIST) {
        // (its upgrade screen: the weapon it upgrades into under the cursor)
        const int into = static_cast<std::int16_t>(rd16(ctx, st + S_INTO_COUNT));
        const int at = static_cast<std::int16_t>(rd16(ctx, st + S_INTO_ROW)) +
                       kIntoMax * static_cast<std::int16_t>(rd16(ctx, st + S_INTO_PAGE));
        if (n.sub != 0 && into > 0 && into <= kIntoMax && at >= 0 && at < into) {
            const std::uint32_t r = st + S_INTO + SLOT * at;
            const int t = Mh4uType(static_cast<int>(rd8(ctx, r)) - 7);
            const int id = static_cast<int>(rd16(ctx, r + 2));
            const int c = Mh3uClass(t);
            if (t >= 0 && c >= 0 && id > 0 && id <= kInfo[c].count) {
                n.type = t, n.id = id, n.rows = into, n.row = at;
                return n;
            }
        }
        // (its grid, and else: the box's weapon under the grid's cursor)
        const std::uint32_t grid = lobby + UPGRADE_GRID;
        const int page = rd8(ctx, grid + G_PAGE), column = rd16(ctx, grid + G_COLUMN), row = rd16(ctx, grid + G_ROW);
        const std::uint32_t user = rd32(ctx, SUSERDATA);
        n.rows = kBoxSlots, n.row = -1;
        if (column < kGridSide && row < kGridSide && page * kGridPage < kBoxSlots && HeapPointer(user)) {
            n.row = page * kGridPage + row * kGridSide + column;
            const std::uint32_t slot = user + BOX + SLOT * n.row;
            const int t = Mh4uType(static_cast<int>(rd8(ctx, slot)) - 7);
            const int id = static_cast<int>(rd16(ctx, slot + 2));
            const int c = Mh3uClass(t);
            if (t >= 0 && c >= 0 && id > 0 && id <= kInfo[c].count) n.type = t, n.id = id;
        }
    }
    return n;
}

}  // namespace mh3u_tables
