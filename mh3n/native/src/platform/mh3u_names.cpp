// MH3U's names for the PC app's Discord Rich Presence, from the player's game (mh3u_names.h).
#include "platform/mh3u_names.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>

#include "mh4n/log.h"
#include "platform/monster_names.h"  // (ArcFile: an MT Framework archive's file, inflated; MH3U's have the same layout)
#include "services/fs/game_romfs.h"

namespace mh4n::platform::mh3u_names {

namespace {

constexpr const char* kLog = "Frontend";

std::uint32_t Le32(const unsigned char* p) {
    return p[0] | p[1] << 8 | p[2] << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}

const char* Folder(int language) {
    static const char* const kFolders[] = {nullptr, "eng", "fre", "spa", "ger", "ita"};
    return language >= 1 && language < 6 ? kFolders[language] : nullptr;
}

/// A text as one line: the game's line breaks become spaces, its tags (<COLO 33>...) go, spaces at the ends too.
std::string OneLine(const std::string& s) {
    std::string out;
    bool tag = false;
    for (const char c : s) {
        if (c == '<') tag = true;
        else if (c == '>' && tag) tag = false;
        else if (!tag) out += (c == '\r' || c == '\n') ? ' ' : c;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    size_t start = 0;
    while (start < out.size() && out[start] == ' ') ++start;
    std::string squeezed;
    for (size_t i = start; i < out.size(); ++i) {
        if (out[i] == ' ' && !squeezed.empty() && squeezed.back() == ' ') continue;
        squeezed += out[i];
    }
    return squeezed;
}

bool ArcTexts(const std::string& arc_path, const std::string& entry, std::vector<std::string>* texts, std::string* why) {
    fs::GameRomFsFile arc;
    if (!fs::OpenGameRomFsFile(arc_path, &arc, why)) return false;
    std::vector<unsigned char> gmd;
    if (!monster_names::ArcFile(arc, entry, &gmd)) return *why = arc_path + " has no " + entry, false;
    if (!GmdTexts(gmd, texts)) return *why = entry + " can't be read", false;
    return true;
}

struct Names {
    std::vector<std::string> maps, towns, monsters, cards;  // (cards: the guild card's backgrounds, full map names)
};
// The maps the guild card's background list names in full (GUI\font\lobby\CardEdit_<l>, 21 to 26): the quest's map id
// and its row there; the others keep the quest list's short name
constexpr std::pair<int, int> kFullMapNames[] = {{1, 21}, {2, 22}, {3, 23}, {13, 24}, {4, 25}, {5, 26}};
std::mutex g_mutex;
std::map<int, Names> g_names;
std::map<int, bool> g_started;
std::map<std::pair<int, int>, std::string> g_quests;  // (language, quest): its title ("" none)

std::string At(const std::vector<std::string>& list, int i) {
    return i >= 0 && i < static_cast<int>(list.size()) ? OneLine(list[i]) : std::string();
}

}  // namespace

bool GmdTexts(const std::vector<unsigned char>& d, std::vector<std::string>* out) {
    // GMD\0, version 0x00010201: +0x0c the labels' count, +0x10 the strings', +0x14 the labels' bytes, +0x18 the
    // strings' bytes, +0x1c the name's length; the name and its NUL at +0x20, 8 bytes a label, the labels' names, then
    // the strings, each ended by a NUL
    if (d.size() < 0x20 || std::memcmp(d.data(), "GMD", 4) != 0) return false;
    const std::uint32_t labels = Le32(d.data() + 0x0c), strings = Le32(d.data() + 0x10);
    const std::uint32_t labels_size = Le32(d.data() + 0x14), strings_size = Le32(d.data() + 0x18);
    const std::uint32_t name = Le32(d.data() + 0x1c);
    if (labels > 65536 || strings > 65536 || name > 4096) return false;
    const std::uint64_t start = 0x20ull + name + 1 + 8ull * labels + labels_size;
    if (start + strings_size > d.size()) return false;
    out->clear();
    std::uint64_t at = start;
    const std::uint64_t end = start + strings_size;
    for (std::uint32_t i = 0; i < strings && at <= end; ++i) {
        std::uint64_t stop = at;
        while (stop < end && d[stop] != 0) ++stop;
        out->emplace_back(reinterpret_cast<const char*>(d.data() + at), static_cast<size_t>(stop - at));
        at = stop + 1;
    }
    return out->size() == strings;
}

bool QuestTitle(const std::vector<unsigned char>& q, int language, std::string* title) {
    // QTDS: u32 0x53445451, u32 5, then the title in eng, fre, ger, ita, spa, each a u32 length and its UTF-8 bytes
    static const int kOrder[] = {-1, 0, 1, 4, 2, 3};  // (the game's language number -> its place in the file)
    if (language < 1 || language > 5 || q.size() < 8 || Le32(q.data()) != 0x53445451u || Le32(q.data() + 4) != 5)
        return false;
    std::uint64_t at = 8;
    for (int i = 0; i < 5; ++i) {
        if (at + 4 > q.size()) return false;
        const std::uint32_t length = Le32(q.data() + at);
        at += 4;
        if (length > 512 || at + length > q.size()) return false;
        if (i == kOrder[language]) {
            std::string s(reinterpret_cast<const char*>(q.data() + at), length);
            while (!s.empty() && s.back() == '\0') s.pop_back();
            *title = OneLine(s);
            return true;
        }
        at += length;
    }
    return false;
}

void Prepare(int language) {
    if (!Folder(language)) return;
    {
        std::lock_guard lock(g_mutex);
        if (g_started[language]) return;
        g_started[language] = true;
    }
    std::thread([language]() {
        const std::string l = Folder(language);
        Names names;
        std::string why_maps, why_towns, why_monsters;
        const auto t0 = std::chrono::steady_clock::now();
        const bool maps = ArcTexts("arc/ID/ID_cock_" + l + ".arc", "GUI\\font\\questdata\\QuestMap_" + l, &names.maps, &why_maps);
        if (!maps && why_maps == "no game runs") {  // (too early: again on a later frame)
            std::lock_guard lock(g_mutex);
            g_started.erase(language);
            return;
        }
        ArcTexts("arc/ID/ID_com_" + l + ".arc", "GUI\\font\\custom\\CUS_HunterSearch_" + l, &names.towns, &why_towns);
        ArcTexts("arc/ID/ID_com_" + l + ".arc", "GUI\\font\\Monster_" + l, &names.monsters, &why_monsters);
        std::string why_cards;
        ArcTexts("arc/ID/ID_lb_" + l + ".arc", "GUI\\font\\lobby\\CardEdit_" + l, &names.cards, &why_cards);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        MH4N_LOG(Info, kLog, "MH3U names in {} ({:.1f} ms): {} maps{}, {} towns{}, {} monsters{}; e.g. {}, {}", l, ms,
                 names.maps.size(), maps ? "" : " (" + why_maps + ")", names.towns.size(),
                 names.towns.empty() ? " (" + why_towns + ")" : "", names.monsters.size(),
                 names.monsters.empty() ? " (" + why_monsters + ")" : "", At(names.maps, 1), At(names.towns, kMogaVillage));
        std::lock_guard lock(g_mutex);
        g_names[language] = std::move(names);
    }).detach();
}

std::string MapName(int language, int map) {
    std::lock_guard lock(g_mutex);
    const auto it = g_names.find(language);
    if (it == g_names.end() || map < 1) return {};
    for (const auto& [id, row] : kFullMapNames) {
        if (id == map) {
            const std::string full = At(it->second.cards, row);
            if (!full.empty()) return full;
        }
    }
    return At(it->second.maps, map);
}

std::string TownName(int language, int town) {
    std::lock_guard lock(g_mutex);
    const auto it = g_names.find(language);
    return it == g_names.end() ? std::string() : At(it->second.towns, town);
}

std::string MonsterName(int language, int id) {
    std::lock_guard lock(g_mutex);
    const auto it = g_names.find(language);
    return it == g_names.end() || id < 1 ? std::string() : At(it->second.monsters, id);
}

std::string QuestName(int language, int quest) {
    if (!Folder(language) || quest <= 0 || quest > 99999) return {};
    {
        std::lock_guard lock(g_mutex);
        const auto it = g_quests.find({language, quest});
        if (it != g_quests.end()) return it->second;
    }
    char path[48];
    std::snprintf(path, sizeof path, "quest/us/q_%05d.quest", quest);
    std::string title, why;
    fs::GameRomFsFile file;
    if (fs::OpenGameRomFsFile(path, &file, &why) && file.size > 0 && file.size < (1u << 20)) {
        std::vector<unsigned char> data(static_cast<size_t>(file.size));
        if (file.Read(0, data.data(), data.size()) == data.size()) QuestTitle(data, language, &title);
    } else if (why == "no game runs") {
        return {};
    }
    std::lock_guard lock(g_mutex);
    g_quests[{language, quest}] = title;  // (a quest not in the RomFS, an event's: none, asked once)
    return title;
}

}  // namespace mh4n::platform::mh3u_names
