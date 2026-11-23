package com.mh4u.port;

/**
 * The game this app runs, in one place: Monster Hunter 3 Ultimate, the console's USA v1.0 (program 00040000000AE400).
 * The MH4U app's classes ask it what the game folder must hold (Dump, GameFolderActivity). MH3U has no update: the base
 * game's code is the one the library is made from, and its RomFS is the only one.
 */
final class Game {
    private Game() {}

    /** The program ids' low words this app runs (the high word is 0x00040000 for the game, 0x0004000E for an update). */
    static final long[] UNIQUE_IDS = {0x000AE400L};
    /** MH4U needs its 1.3.0 update beside the game; MH3U has none. */
    static final boolean NEEDS_UPDATE = false;
    /** The game's title as Azahar names its folders (mods, saves). */
    static final String TITLE = "00040000000AE400";
    static final String NAME = "Monster Hunter 3 Ultimate";
    /** The short name the app's texts use. */
    static final String SHORT = "MH3U";

    // ---- the save (the game's own files, as Azahar and Citra keep them; Checkpoint's backup holds the same)
    /** Its title folder: title/00040000/<TITLE_LOW>/data/00000001. */
    static final String TITLE_LOW = "000ae400";
    /** Its extra data (guild cards, event quests): extdata/00000000/<EXT_ID>. */
    static final String EXT_ID = "00000AE4";
    /** The save's files: system and a file a hunter (user2 and user3 only once those slots are used). */
    static final String[] SAVE_FILES = {"system", "user1", "user2", "user3"};
    /** Their sizes as the game writes them: system's exactly; a hunter's file at least this (a shorter one is cut). */
    static final int SYSTEM_SIZE = 12288, USER_SIZE = 35328;
    /** The save archive's format record (FormatSaveData's values for MH3U: 32 folders, 32 files, duplicate data). */
    static final byte[] SAVE_METADATA = {0, 0, 4, 0, 32, 0, 0, 0, 32, 0, 0, 0, 1, 0, 0, 0};
    /** A backup tool's folder name for it (Checkpoint "0x00AE4 Monster Hunter 3 Ultimate"), lower case. */
    static boolean backupFolder(String lower) {
        return lower.startsWith("0x00ae4") || lower.contains("monster hunter 3");
    }

    static boolean ours(long programId) {
        long unique = programId & 0xFFFFFFFFL;
        for (long id : UNIQUE_IDS) if (unique == id) return true;
        return false;
    }
}
