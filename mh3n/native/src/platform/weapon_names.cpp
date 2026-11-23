// The weapons' names for the weapon tree, from the game's RomFS (weapon_names.h).
#include "platform/weapon_names.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>

#include "mh4n/log.h"
#include "mh4n/registry.h"
#include "mh4n/runtime.h"
#include "platform/monster_names.h"
#include "services/fs/game_romfs.h"
#include "hunt_aids.h"  // (HuntAids::IsMh3u: the game the library is)
#include "weapon_tree.h"

namespace mh4n::platform::weapon_names {

namespace {

constexpr const char* kLog = "Frontend";
// each class's message file, in equipment type order (7 ... 20)
constexpr const char* kFiles[] = {"LswordName", "SwordName", "HammerName", "LanceName", "LightName", "HeavyName",
                                  "Lsword2Name", "AxeName", "Lance2Name", "BowName", "WswordName", "Hammer2Name",
                                  "RodName", "GaxeName"};
constexpr int kFirstType = 7, kClasses = 14;
// the weapons' cells of the icon sheet's large icons, in weapon_tree_panel.h's WeaponClass order (the game's table at
// 0x00fb811b, by equipment type 7 ... 20: 55 57 59 61 63 65 56 67 62 66 58 60 68 69)
constexpr int kIconCells[kClasses] = {55, 56, 57, 58, 59, 60, 61, 62, 63, 65, 66, 67, 68, 69};
constexpr int kIconSide = 96;  // a cell as the app gets it (the sheet's 24 pixels, each made 4 x 4)
// MH3U's: the weapons' cells of its icon sheet, in the same order (-1: the two classes it doesn't have). The game's own
// table at 0x00392424 (MH3U's code), by its class 0 ... 12 (weapon_tree_mh3u.inc's): 72 71 75 74 78 77 76 73 79 82 83
// 80 81 (77: the class 5 it doesn't use); the function just before it, at 0x0039237c, turns a cell's number into its
// place on the sheet.
constexpr int kMh3uIconCells[kClasses] = {72, 73, 71, 80, 75, 81, 74, 82, 76, 78, 83, 79, -1, -1};

std::mutex g_mutex;
std::map<int, std::vector<std::vector<std::string>>> g_names;  // (a language read; empty while being read, or none)
std::map<int, std::vector<std::string>> g_items;               // (the items' names, by id)
std::map<int, std::vector<std::uint8_t>> g_icons;              // (the classes' icons, kIconSide a side, side by side)
std::map<int, bool> g_started;

std::string Trim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n' || s.back() == '\r')) s.pop_back();
    std::size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    return s.substr(i);
}

// The weapons' icons of the game's icon sheet <language>\lyt\common\texture\cmn_icon_GSM_NOMIP (a TEX: +8 the
// levels in bits 0-5, the width in 6-18 and the height in 19-31, +12 the format in bits 8-15, 0x10 the console's IA4;
// one level, its texels the last width x height bytes): its large icons from y 208, 24 pixels a cell, ten a row, which
// the game tints by rarity. Each weapon's cell, made kIconSide a side with whole pixels, side by side, RGBA.
bool ReadIcons(const fs::GameRomFsFile& arc, const char* lang, std::vector<std::uint8_t>* out) {
    std::vector<unsigned char> t;
    if (!monster_names::ArcFile(arc, std::string(lang) + "\\lyt\\common\\texture\\cmn_icon_GSM_NOMIP", &t) || t.size() < 20 ||
        std::memcmp(t.data(), "TEX", 4) != 0)
        return false;
    auto le32 = [&](std::size_t at) { return std::uint32_t{t[at]} | t[at + 1] << 8 | t[at + 2] << 16 | std::uint32_t{t[at + 3]} << 24; };
    const std::uint32_t b = le32(8), c = le32(12);
    const std::uint32_t w = (b >> 6) & 0x1fff, h = (b >> 19) & 0x1fff;
    if ((b & 0x3f) != 1 || ((c >> 8) & 0xff) != 0x10 || w != 256 || h != 512 || t.size() < 16 + std::size_t{w} * h) return false;
    // (IA4: a byte a texel, its intensity in the high four bits and its alpha in the low four; 8 x 8 tiles in rows, the
    // texels of a tile in Morton order)
    const unsigned char* texels = t.data() + t.size() - std::size_t{w} * h;
    auto texel = [&](std::uint32_t x, std::uint32_t y, std::uint8_t* out) {
        std::uint32_t m = 0;
        for (int bit = 0; bit < 3; ++bit) m |= ((x >> bit) & 1) << (2 * bit) | ((y >> bit) & 1) << (2 * bit + 1);
        const unsigned char v = texels[((y / 8) * (w / 8) + x / 8) * 64 + m];
        out[0] = out[1] = out[2] = static_cast<std::uint8_t>((v >> 4) * 17), out[3] = static_cast<std::uint8_t>((v & 15) * 17);
    };
    constexpr int cell = 24, top = 208, k = kIconSide / cell;
    out->assign(std::size_t{kIconSide} * kIconSide * kClasses * 4, 0);
    for (int j = 0; j < kClasses; ++j) {
        const int cx = kIconCells[j] % 10 * cell, cy = top + kIconCells[j] / 10 * cell;
        for (int y = 0; y < kIconSide; ++y)
            for (int x = 0; x < kIconSide; ++x)
                texel(cx + x / k, cy + y / k, &(*out)[(std::size_t(y) * kIconSide * kClasses + j * kIconSide + x) * 4]);
    }
    return true;
}

// MH3U's weapons' icons, from its icon sheet GUI\Texture\common\td_icon_ID_<language> in arc/ID/ID_com_<language>.arc
// (a TEX with MH4U's header, its format 0x0c the console's ETC1A4: 512 x 512, one level, a byte a texel, the texels the
// last width x height bytes): its item and equipment icons from the top left, 22 pixels a cell, ten a row (cell n at
// n % 10 and n / 10 times 22, as the game's function at 0x0039237c places it), grey shapes the game tints by rarity
// (the same cells in every language). Each weapon's cell made four times as large (88 pixels) in the middle of a
// kIconSide one, side by side, RGBA, as ReadIcons gives MH4U's; the classes MH3U doesn't have stay clear.
bool ReadIconsMh3u(const fs::GameRomFsFile& arc, const char* lang, std::vector<std::uint8_t>* out) {
    std::vector<unsigned char> t;
    if (!monster_names::ArcFile(arc, std::string("GUI\\Texture\\common\\td_icon_ID_") + lang, &t) || t.size() < 20 ||
        std::memcmp(t.data(), "TEX", 4) != 0)
        return false;
    auto le32 = [&](std::size_t at) { return std::uint32_t{t[at]} | t[at + 1] << 8 | t[at + 2] << 16 | std::uint32_t{t[at + 3]} << 24; };
    const std::uint32_t b = le32(8), c = le32(12);
    const std::uint32_t w = (b >> 6) & 0x1fff, h = (b >> 19) & 0x1fff;
    if ((b & 0x3f) != 1 || ((c >> 8) & 0xff) != 0x0c || w != 512 || h != 512 || t.size() < 16 + std::size_t{w} * h) return false;
    // (ETC1A4: 8 x 8 tiles in rows, each four 4 x 4 blocks of 16 bytes, top left, top right, bottom left, bottom
    // right: the alpha, a 64-bit little-endian number of four bits a texel, then the colour, an ETC1 block as a 64-bit
    // little-endian number: bits 0-15 the low bit of each texel's modifier, 16-31 its sign, 32 the flip, 33 the
    // differential mode, 34-36 and 37-39 the two halves' tables, 40-63 their base colours. A block's texel (x, y) is
    // its number 4 x + y.) ETC1's modifiers, by table:
    static constexpr int kModifier[8][2] = {{2, 8},   {5, 17},  {9, 29},  {13, 42},
                                            {18, 60}, {24, 80}, {33, 106}, {47, 183}};
    const unsigned char* texels = t.data() + t.size() - std::size_t{w} * h;
    auto le64 = [](const unsigned char* p) {
        std::uint64_t v = 0;
        for (int i = 7; i >= 0; --i) v = v << 8 | p[i];
        return v;
    };
    auto texel = [&](std::uint32_t x, std::uint32_t y, std::uint8_t* px) {
        const unsigned char* block = texels + ((y / 8) * (w / 8) + x / 8) * 64 + (x / 4 % 2 + y / 4 % 2 * 2) * 16;
        const std::uint64_t alpha = le64(block), e = le64(block + 8);
        const std::uint32_t i = x % 4 * 4 + y % 4;
        // (the block's second half: its bottom two rows when flipped, else its right two columns)
        const bool second = (((e >> 32) & 1) != 0 ? y % 4 : x % 4) >= 2;
        int m = kModifier[(e >> (second ? 34 : 37)) & 7][(e >> i) & 1];
        if (((e >> (16 + i)) & 1) != 0) m = -m;
        for (int ch = 0; ch < 3; ++ch) {  // (red's base colours at bits 56-63, green's 48-55, blue's 40-47)
            const int at = 56 - 8 * ch;
            int base;
            if (((e >> 33) & 1) != 0) {  // (differential: five bits, the second half's those plus three signed ones)
                int v = static_cast<int>((e >> (at + 3)) & 31);
                if (second) {
                    const int d = static_cast<int>((e >> at) & 7);
                    v = (v + (d >= 4 ? d - 8 : d)) & 31;
                }
                base = v << 3 | v >> 2;
            } else {  // (four bits each half)
                base = static_cast<int>((e >> (second ? at : at + 4)) & 15) * 17;
            }
            px[ch] = static_cast<std::uint8_t>(std::clamp(base + m, 0, 255));
        }
        px[3] = static_cast<std::uint8_t>(((alpha >> (4 * i)) & 15) * 17);
    };
    constexpr int cell = 22, k = 4, margin = (kIconSide - cell * k) / 2;
    out->assign(std::size_t{kIconSide} * kIconSide * kClasses * 4, 0);
    for (int j = 0; j < kClasses; ++j) {
        if (kMh3uIconCells[j] < 0) continue;
        const int cx = kMh3uIconCells[j] % 10 * cell, cy = kMh3uIconCells[j] / 10 * cell;
        for (int y = 0; y < cell * k; ++y)
            for (int x = 0; x < cell * k; ++x)
                texel(cx + x / k, cy + y / k,
                      &(*out)[(std::size_t(margin + y) * kIconSide * kClasses + j * kIconSide + margin + x) * 4]);
    }
    return true;
}

// The items' names (the recipes' materials), one text an id: <language>\msg\itemName_<language>
bool ReadItems(const fs::GameRomFsFile& arc, const char* lang, std::vector<std::string>* out) {
    std::vector<unsigned char> lmd;
    if (!monster_names::ArcFile(arc, std::string(lang) + "\\msg\\itemName_" + lang, &lmd)) return false;
    if (!monster_names::LmdTexts(lmd, out)) return false;
    for (std::string& s : *out) s = Trim(s);
    return true;
}

}  // namespace

// MH3U: each class's names in arc/ID/msg/ID_weaponmsg_<lang>.arc, GUI\font\<file>_<lang> (GMD files), in equipment
// type order (the port's 7 ... 20, MH4U's: weapon_tree.h); none for the two classes MH3U doesn't have
constexpr const char* kMh3uFiles[kClasses] = {"Lsword", "Sword", "Hammer", "Lance", "Lbg", "Hbg", "Lsword2",
                                              "Axe", "Gunlance", "Bow", "WSword", "Pipe", nullptr, nullptr};

bool ReadMh3u(const char* lang, std::vector<std::vector<std::string>>* names, std::vector<std::string>* items,
              std::string* why) {
    const std::string arc_path = std::string("arc/ID/msg/ID_weaponmsg_") + lang + ".arc";
    fs::GameRomFsFile arc;
    if (!fs::OpenGameRomFsFile(arc_path, &arc, why)) return false;
    names->assign(kClasses, {});
    for (int c = 0; c < kClasses; ++c) {
        if (kMh3uFiles[c] == nullptr) continue;
        const std::string file = std::string("GUI\\font\\") + kMh3uFiles[c] + "_" + lang;
        std::vector<unsigned char> gmd;
        if (!monster_names::ArcFile(arc, file, &gmd)) return *why = arc_path + " has no " + file, false;
        if (!monster_names::GmdTexts(gmd, &(*names)[c])) return *why = file + " can't be read", false;
        for (std::string& n : (*names)[c]) n = Trim(n);
    }
    // the items (the recipes' materials): arc/ID/ID_com_<lang>.arc, GUI\font\Item00_<lang>, one text an id
    fs::GameRomFsFile com;
    std::string ignored;
    std::vector<unsigned char> gmd;
    items->clear();
    if (fs::OpenGameRomFsFile(std::string("arc/ID/ID_com_") + lang + ".arc", &com, &ignored) &&
        monster_names::ArcFile(com, std::string("GUI\\font\\Item00_") + lang, &gmd) && monster_names::GmdTexts(gmd, items)) {
        for (std::string& n : *items) n = Trim(n);
    } else {
        items->clear();
    }
    *why = arc_path + " of the game's RomFS";
    return true;
}

bool Read(int language, std::vector<std::vector<std::string>>* names, std::string* why) {
    const char* lang = monster_names::Folder(language);
    if (!lang) return *why = "no texts in that language", false;
    if (Port::HuntAids::IsMh3u()) {
        std::vector<std::string> items;
        return ReadMh3u(lang, names, &items, why);
    }
    const std::string arc_path = std::string(lang) + "/data/core_common.arc";
    fs::GameRomFsFile arc;
    if (!fs::OpenGameRomFsFile(arc_path, &arc, why)) return false;
    const char* where = arc.update ? "the update's RomFS" : "the game's RomFS";
    names->assign(kClasses, {});
    for (int c = 0; c < kClasses; ++c) {
        const std::string file = std::string(lang) + "\\msg\\" + kFiles[c] + "_" + lang;
        std::vector<unsigned char> lmd;
        if (!monster_names::ArcFile(arc, file, &lmd)) return *why = arc_path + " of " + where + " has no " + file, false;
        if (!monster_names::LmdTexts(lmd, &(*names)[c])) return *why = file + " of " + where + " can't be read", false;
        for (std::string& s : (*names)[c]) s = Trim(s);
    }
    *why = arc_path + " of " + where;
    return true;
}

void Prepare(int language) {
    if (!monster_names::Folder(language)) return;
    {
        std::lock_guard lock(g_mutex);
        if (g_started[language]) return;
        g_started[language] = true;
    }
    std::thread([language]() {
        std::vector<std::vector<std::string>> names;
        std::vector<std::string> items;
        std::vector<std::uint8_t> icons;
        std::string why;
        const auto t0 = std::chrono::steady_clock::now();
        const bool mh3u = Port::HuntAids::IsMh3u();
        const bool ok = mh3u ? ReadMh3u(monster_names::Folder(language), &names, &items, &why) : Read(language, &names, &why);
        if (ok && !mh3u) {  // (the recipes' items and the classes' icons, from the same file)
            const char* lang = monster_names::Folder(language);
            fs::GameRomFsFile arc;
            std::string why2;
            if (fs::OpenGameRomFsFile(std::string(lang) + "/data/core_common.arc", &arc, &why2)) {
                if (!ReadItems(arc, lang, &items)) items.clear();
                if (!ReadIcons(arc, lang, &icons)) icons.clear();
            }
        } else if (ok) {  // (MH3U's icons: its icon sheet, in the file of its items' names)
            const char* lang = monster_names::Folder(language);
            fs::GameRomFsFile com;
            std::string why2;
            if (!fs::OpenGameRomFsFile(std::string("arc/ID/ID_com_") + lang + ".arc", &com, &why2) ||
                !ReadIconsMh3u(com, lang, &icons))
                icons.clear();
        }
        if (!ok && why == "no game runs") {  // (too early: again on a later call)
            std::lock_guard lock(g_mutex);
            g_started.erase(language);
            return;
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (ok) {
            std::size_t count = 0;
            for (const auto& c : names) count += c.size();
            MH4N_LOG(Info, kLog, "weapon names: {} in {}, from {} ({:.1f} ms): {} ... {}; {} items, {}", count,
                     monster_names::Folder(language), why, ms, names[0].size() > 1 ? names[0][1] : std::string(),
                     names.back().empty() ? std::string() : names.back().back(), items.size(),
                     icons.empty() ? "no icons (drawn ones)" : "the game's icons");
        } else {
            MH4N_LOG(Warning, kLog, "weapon names: none in {} ({})", monster_names::Folder(language), why);
        }
        std::lock_guard lock(g_mutex);
        g_names[language] = std::move(names);
        g_items[language] = std::move(items);
        g_icons[language] = std::move(icons);
    }).detach();
}

std::string Item(int language, int id) {
    std::lock_guard lock(g_mutex);
    const auto it = g_items.find(language);
    if (it == g_items.end() || id <= 0 || id >= static_cast<int>(it->second.size())) return {};
    return it->second[id];
}

bool Icons(int language, std::vector<std::uint8_t>* rgba, int* cell) {
    std::lock_guard lock(g_mutex);
    const auto it = g_icons.find(language);
    if (it == g_icons.end() || it->second.empty()) return false;
    *rgba = it->second;
    *cell = kIconSide;
    return true;
}

std::string Name(int language, int type, int id) {
    std::lock_guard lock(g_mutex);
    const auto it = g_names.find(language);
    const int c = type - kFirstType;
    if (it == g_names.end() || c < 0 || c >= static_cast<int>(it->second.size())) return {};
    const auto& list = it->second[c];
    if (id <= 0 || id >= static_cast<int>(list.size())) return {};
    return list[id];
}

namespace {

// Phase `started`: the weapon tree (port/src/weapon_tree.cpp) takes its names from here; it asks for a language's when
// the Smithy first opens.
void StartedStep(Runtime&) {
    Port::WeaponTree::SetNames(&Name, &Prepare, &Item, &Icons);
}

MH4N_REGISTER(BootStep, "platform.weapon_names", BootStep{Phase::Started, StartedStep});

}  // namespace

}  // namespace mh4n::platform::weapon_names
