// MH3U's names for the PC app's Discord Rich Presence (Port::Presence::Info of an MH3U game): the maps, the towns, the
// quests and the monsters, in the language the game shows its texts in, read from the player's own game (its RomFS, as
// the game reads it: services/fs/game_romfs.h) on a thread of their own, once a language. None of them is in the
// program. MH3U keeps its texts in GMD message files (rGUIMessage: UTF-8 strings) inside MT Framework archives:
//   the maps:     arc/ID/ID_cock_<l>.arc, GUI\font\questdata\QuestMap_<l> (string 1.. the quest's map id)
//   the towns:    arc/ID/ID_com_<l>.arc, GUI\font\custom\CUS_HunterSearch_<l> (21 Moga Village, 22 Port Tanzia, 25 Moga Woods)
//   the monsters: arc/ID/ID_com_<l>.arc, GUI\font\Monster_<l> (string n the monster of id n)
//   a quest:      quest/us/q_<id, 5 digits>.quest (QTDS: its title in eng, fre, ger, ita, spa)
// <l> is eng, fre, spa, ger or ita: the game's language number 1 to 5 (sGUI + 0x1c), as MH4U's.
#pragma once

#include <string>
#include <vector>

namespace mh4n::platform::mh3u_names {

enum Town { kMogaVillage = 21, kPortTanzia = 22, kMogaWoods = 25 };

// Starts reading the names of `language` (1 to 5), once. Any thread.
void Prepare(int language);

// The names (UTF-8, one line: the game's line breaks and tags taken out); empty while being read, or with none. Any thread.
std::string MapName(int language, int map);
std::string TownName(int language, int town);
std::string MonsterName(int language, int id);
// A quest's title, read the first time it is asked (on the caller's thread: a small file).
std::string QuestName(int language, int quest);

// (tests) the strings of a GMD message file, in their order; false when it isn't one.
bool GmdTexts(const std::vector<unsigned char>& gmd, std::vector<std::string>* texts);
// (tests) a QTDS quest file's title in the game's language `language` (1 to 5).
bool QuestTitle(const std::vector<unsigned char>& quest, int language, std::string* title);

}  // namespace mh4n::platform::mh3u_names
