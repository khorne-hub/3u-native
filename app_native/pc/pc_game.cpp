// MH3U for PC: the game's files (pc_game.h). What a file of the game folder is (the game, an update, another game), and
// the game's small files in the home's game/ that the runtime loads (exheader.bin, code.bin decompressed, icon.bin, and
// source.txt, through which it reads the RomFS in place from the player's own file). MH3U has no update: the game's own
// code is the one that runs, from the game alone, as the Android app's Dump.prepareBase makes it.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "common/utf8.h"
#include "pc_app.h"
#include "pc_game.h"
#include "services/fs/dump_source.h"
#include "services/fs/positioned_file.h"

namespace fs = std::filesystem;

std::string pc_game_describe(const std::string& path, unsigned long long* program) {
    std::string error;
    auto file = mh4n::fs::PositionedFile::Open(path, &error);
    if (!file) return "";
    mh4n::fs::dump::Ncch n;
    if (!mh4n::fs::dump::FindNcch(*file, &n, &error)) return "";
    if (program) *program = n.program_id;
    const unsigned long long low = n.program_id & 0xFFFFFFFFull, high = n.program_id >> 32;
    if (!pc_game_title(low)) return high == 0x00040000ull ? "other" : "";
    if (high == 0x00040000ull) return "game";
    if (high == 0x0004000Eull) return "update";
    return "";
}

std::string pc_game_prepare(const std::string& base, const std::string& update, const std::string& folder) {
    std::string e;
    if (kPcGameHasUpdate || !update.empty()) {  // (a game with an update: the runtime's own, MH4U's way)
        return mh4n::fs::dump::PrepareGameFolder(base, update, folder, &e) ? "" : e;
    }
    auto file = mh4n::fs::PositionedFile::Open(base, &e);
    if (!file) return e;
    mh4n::fs::dump::Ncch game;
    if (!mh4n::fs::dump::FindNcch(*file, &game, &e)) return "the game: " + e;
    if (!game.no_crypto) return "the game is encrypted: it has to be a decrypted copy of your own";
    if ((game.program_id >> 32) != 0x00040000ull || !pc_game_title(game.program_id & 0xFFFFFFFFull)) {
        char id[40];
        std::snprintf(id, sizeof id, "%016llX", static_cast<unsigned long long>(game.program_id));
        return std::string("the game is not " PC_GAME_TITLE " (title ") + id + ")";
    }
    if (game.exheader_size == 0) return "the game has no exheader";
    std::vector<unsigned char> exheader(0x800), code, icon;
    bool failed = false;
    if (file->ReadAt(game.base + 0x200, exheader.data(), exheader.size(), &failed) != exheader.size())
        return failed ? "the game could not be read" : "the game ends too early (a copy cut short?)";
    if (!mh4n::fs::dump::ReadExeFsFile(*file, game, ".code", &code, &e)) return "the game: " + e;
    if ((exheader[0x0D] & 1) && !mh4n::fs::dump::BlzDecompress(code, &e)) return "the game: " + e;
    const bool has_icon = mh4n::fs::dump::ReadExeFsFile(*file, game, "icon", &icon, &e);

    std::error_code ec;
    const fs::path dir = mh4n::utf8::PathFromUtf8(folder);
    fs::create_directories(dir, ec);
    auto write = [&](const char* name, const std::vector<unsigned char>& bytes) {
        std::ofstream o(dir / name, std::ios::binary | std::ios::trunc);
        o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(o);
    };
    if (!write("exheader.bin", exheader) || !write("code.bin", code) || (has_icon && !write("icon.bin", icon)))
        return "the game's files could not be written in " + folder;
    if (!has_icon) fs::remove(dir / "icon.bin", ec);
    fs::remove(dir / "romfs.bin", ec);
    fs::remove(dir / "update_romfs.bin", ec);
    {
        std::ofstream s(dir / "source.txt", std::ios::trunc);
        s << "base=" << base << "\n";
    }
    {
        char text[512];
        std::snprintf(text, sizeof text, "base game     : program %016llX (no update)\nicon source   : %s\ninput base  : %s, 0x%llX bytes\n",
                      static_cast<unsigned long long>(game.program_id), has_icon ? "game" : "none", base.c_str(),
                      static_cast<unsigned long long>(file->Size()));
        std::ofstream i(dir / "info.txt", std::ios::trunc);
        i << text;
    }
    return "";
}
