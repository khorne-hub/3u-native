// MH3U for PC: the game this program runs, in one place. The PC program around it (pc/*.cpp) is the MH4U port's, the
// same files in both ports; what belongs to one game is here: the program's name (MH3U.exe, its windows, its folders in
// %APPDATA% and %LOCALAPPDATA%, which must not be MH4U for PC's), the game's program ids, whether it has an update, the
// code library's name, and which of the MH4U port's features (mh3n port/src) have this game's addresses yet.
#pragma once

#include <string>

// The program's name: MH3U.exe, its windows' titles, %APPDATA%\MH3U (the saves) and %LOCALAPPDATA%\MH3U (the pipeline cache)
#define PC_GAME "MH3U"
// The game's whole name (Discord's text, the guides)
#define PC_GAME_TITLE "Monster Hunter 3 Ultimate"
// The code library each player makes from their own game (mh3n tools/build_game.py --pc, PC_LIBRARY there), and its id
// file beside it
#define PC_GAME_LIBRARY "mh3u_code_native.dll"
#define PC_GAME_LIBRARY_ID "mh3u_code_native.id"
// The project's repository on GitHub, its guide and its releases (the update notice asks its latest release). A name
// proposed for MH3U's public repository, not made yet: the owner decides it before a release (until then the notice
// finds no release and says nothing)
#define PC_GAME_REPO "3u-native"

// The game's program ids' low words (the high word is 00040000): USA v1.0, 00040000000AE400. The game has no update
// (none on its card, none known for this region): a game folder holds the game alone, and the game's own code is the one
// recompiled.
constexpr unsigned long long kPcGameTitles[] = {0x000AE400ull};
constexpr const char* kPcGameProgram = "00040000000AE400";
constexpr const char* kPcGameUpdateProgram = "0004000E000AE400";  // (the form an update's title folder would take)
constexpr bool kPcGameHasUpdate = false;
// The game's save, as the console keeps it: title/00040000/<kPcGameSaveTitle>/data/00000001 (system, and user1..3, the
// three hunters), its format record (00000001.metadata: FormatSaveData's values, the game's own), and its extra data
// extdata/00000000/<kPcGameExtdata> (card1..3 and cardbox, the guild cards; quest1..4, the event quests). JKSM names a
// title's backups by kPcGameJksm. A system or user file of another size isn't this game's.
constexpr const char* kPcGameSaveTitle = "000ae400";
constexpr const char* kPcGameExtdata = "00000ae4";
constexpr const char* kPcGameJksm = "0x00ae4";
constexpr unsigned char kPcGameSaveMetadata[16] = {0, 0, 4, 0, 0x20, 0, 0, 0, 0x20, 0, 0, 0, 1, 0, 0, 0};
constexpr unsigned long long kPcGameSystemSize = 12288, kPcGameUserSize = 35328;  // (MH3U's, as the game writes them)
// The other game's saves (MH4U's titles: USA, Europe, Japan's MH4G), told apart by name when a folder holds one
constexpr const char* kPcOtherSaveTitles[] = {"00126300", "00126100", "0011d700"};
constexpr const char* kPcOtherExtdata[] = {"00001263", "00001261", "000011d7"};
// The RomFS's top folders (MH3U's has names: arc/, quest/, sound/...): a mod's loose game folders are found by them
constexpr const char* kPcGameRomfsTop[] = {"arc", "quest", "sound", "pop", "movie", "GUI", "system", "LtUserShader"};

// The MH4U port's features that read or call the game (mh3n port/src: widescreen and the HUD at the edges, one screen,
// quick items, the hunting aids, the weapon tree, gathering, Discord's places). An MH3U library has none of their hook
// points or addresses, so none of them runs; these say so to the PC's own rows and screens, which hide what would do
// nothing. MH3U's own versions of them come with its addresses (stage 4), and each turns on here.
constexpr bool kPcGameWidescreen = true;   // the top screen drawn wider than 400x240 (the PC's HUD anchor needs it):
                                            // mh3n port/src/mh3u_gfx.inc
constexpr bool kPcGamePresence = true;     // where the hunter is, the quest and the monsters (Discord's details:
                                           // mh3n port/src/mh3u_qol.cpp, Presence::Info game 3)
constexpr bool kPcGameTouchActions = false; // MH4U's touch-screen actions all in one (the town's SELECT list, the quest's
                                            // item shortcuts panel, the item bar's turn by its own hook): MH3U has no
                                            // item shortcuts panel nor that hook, so its own versions are the flags
                                            // below, each on as it works (town actions, item pouch key, item bar)
constexpr bool kPcGameWeaponTree = true;    // the Smithy's weapon tree (the right stick's click there, F2): MH3U's
                                            // since mh3u-caza (port/src/weapon_tree_mh3u.inc)
constexpr bool kPcGamePadActions = true;    // the port's target camera button (ZL: MH3U's Target Cam stepped through the
                                            // large monsters, mh3n mh3u_qol.cpp TargetStep) and the right stick's click
                                            // (Alt) locking on (F2 says them so)
constexpr bool kPcGameKick = false;         // the port's kick button (ZR): MH3U's touch panel has no kick, so ZR stays
                                            // the game's own ZR (and the end of the wait with ZL)
// (mh3u-ui) MH3U's own versions (mh3n port/src/mh3u*.cpp, app_native/town.cpp, game_screen.cpp), each on as it works:
constexpr bool kPcGameTownActions = true;     // the town panel's Guild Cards, Gestures, Items and Hunters for Hire (SELECT's
                                              // list, their keys, F2), tapped where MH3U's Hunter Search panel has them
constexpr bool kPcGameItemShortcuts = false;  // MH4U's item shortcuts panel (keys 1 to 4): MH3U has no such panel
constexpr bool kPcGameItemPouchKey = true;    // MH3U's Item Pouch panel of the quest's touch panels: its key (1) taps it,
                                              // which opens the pouch (mh3n mh3u_panels.cpp says where the panel is)
constexpr bool kPcGameItemBar = true;         // the item bar's turn (its keys, the wheel): by L with Y or A, the game's own

// A program id's low word is this game's
constexpr bool pc_game_title(unsigned long long low) {
    for (unsigned long long t : kPcGameTitles)
        if (t == low) return true;
    return false;
}

// (pc_game.cpp) What a game folder's file is, by the title in it (an NCSD's partition 0, a CIA's content 0, as the
// Android app's Dump.describe): "game" (this game), "update" (its update), "other" (another game; *program says which)
// or "" (not a game)
std::string pc_game_describe(const std::string& path, unsigned long long* program = nullptr);
// The game's small files into `folder` (the home's game/) from the game, and its update for a game that has one: "" or
// what went wrong
std::string pc_game_prepare(const std::string& base, const std::string& update, const std::string& folder);
