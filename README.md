# MH3U for Android and PC

Play your own copy of Monster Hunter 3 Ultimate on Android, natively and with touch controls made for phones, and on a
Windows PC with keyboard, mouse or a controller ([PC (Windows)](#pc-windows)).
It is not an emulator: the game's code is recompiled ahead of time into native code for your device, and runs on a
runtime of its own that does the console's work, with its graphics drawn through Vulkan.

**This project includes no game code, game files, ROMs or keys. You need your own copy of the game.**

MH3U for Android and PC is the third port of the series, made the same way as
[MH4U for Android and PC](https://github.com/khorne-hub/4u-native) and
[MHGU for Android and PC](https://github.com/khorne-hub/gu-native), with everything MH4U for Android and PC 1.0.2 has that
exists in MH3U: the same app, settings, controls, multiplayer and comforts, on MH3U's own game. The three ports go together.

[Lee esta guía en español](README.es.md)

Talk with other players and ask for help in the community's [Discord](https://discord.gg/sk5xEjEMwF).

## What it does

### Display and performance

- **Your phone's own resolution:** the 3D picture is drawn at your screen's resolution by default (three times the
  console's on a 720-line screen). Lower it under the gear, **Graphics & Audio**, **3D resolution**, down to the
  console's own
- **Widescreen that matches your screen**, from ultra-wide phones to 4:3 handhelds, with no bars, and the **HUD at the
  screen's edges**
- **15 to 240 fps:** the **Frame rate limit** goes from 15 to 240. The game keeps its speed; only more or fewer frames are
  drawn. **Steady frame rate**, off by default, runs an even part of the limit when the phone can't hold it
- **Low-end mode** for weak phones, with **Toaster** (a steady 30 fps, the console's resolution, no reverb) and **Brick**
  (the console's resolution, flat textures and sharp pixels, for the weakest) as its other choices
- Switches of their own for shadows, the game's particle **effects**, each of its screen filters (bloom, motion blur,
  depth of field, radial blur, heat haze) and reverb, and the **field of view**
- **Frame generation**, **FSR 1** with its sharpness, **Fast scaling (more FPS)**, the **PS1 filter**, **Colour** (hue,
  temperature, tint, brightness, contrast, saturation, gamma) with three presets, **HD texture packs** (also in KTX2 and
  at half size) and **your own GPU driver**, at your own risk
- An optional FPS counter, with the phone's temperature, that the layout editor moves anywhere
- **Single-screen mode, as MH4U's:** the game itself draws the touch screen's menus on the main screen, as sharp as the
  rest of the picture and beside what they belong to: the main menu, the save's choice, character creation, the START
  menu and its Item Pouch, the item boxes, the supply box, the shops with how many and the total, the Smithy and the
  Armory, the quest counters and the Tavern's Quest Board, the rewards, the Cha-Cha and Kayamba menus, the farm. You work
  them with the buttons, the menu pad or a slide, as in MH4U; OK and Back are A and B. The touch screen stays hidden, and
  its button brings it back for what still needs a finger (the town's actions and a quest's quick items open large)
- **The app in the colours of water:** the settings, the buttons, the editor and the app's screens in the sea green and
  gold of MH3U's icon
- The touch screen can also go beside the game, under it, or in a corner (moved, resized, see-through or hidden), or on
  the second screen of a device with two

### Touch controls

- A stick, the camera by dragging, a pill of attacks, and buttons to roll, guard, sheathe, target, lock on and centre the
  camera
- **The controls follow the game:** the hunt, the menus, the talks, and bows and bowguns with a layout of their own
  (a large shot button, reload, aim, coatings and ammo)
- **The menu pad:** a D-pad with L and R under your thumb in the game's menus, also in the windows that ask how many
- **The item bar without the D-pad:** a tap uses the item, a slide turns the bar, and its arrows follow it
- **Lock on:** the camera keeps turning to the monster (the Target Cam's, or the nearest large one), also under water;
  bows and bowguns turn to it when they shoot
- **The map as a piece of the HUD:** the area map, with the hunter's arrow and the monsters' icons, moves, grows, shrinks
  and hides as one; a **Map** button shows or hides it
- **Customize controls and HUD:** move and resize every button and every part of the HUD (the bars, the clock, the
  hunter's name, the map, the items, the ammo), each part's frame right on it at any screen shape, and **HUD size** from
  60 to 100 %. At 4:3 and with any field of view the HUD stays whole and in its place
- **10 control profiles** (the app's own layout, or an emulator-like one), exported and imported as a `.zip`;
  **15 combined buttons** per profile; buttons that stay pressed (**Hold**), and **your own pictures** on the buttons
- **Aim with the gyroscope** while a bowgun aims; the camera on the right half only; a floating stick; hide the controls
  with a long press on an empty place
- **The phone's keyboard** for your hunter's name and the chat
- **The camera up and down, on land too:** dragging up or down moves the camera smoothly through the game's six heights
  (the D-pad's steps, without the steps); under water it turns in every direction and swims you up and down, and the air
  gauge shows

### Controllers and keyboard

- A controller hides the touch controls; **START+SELECT** opens the settings
- Without the Circle Pad Pro, the right stick turns the camera, and its click locks on
- The town's actions (Guild Cards, Gestures, Items, Hunters for Hire) without the touch screen
- Buttons by their place or by their letters, the camera's speed, the dead zone, and **Map a controller** for one the game
  doesn't read well
- **A keyboard and a mouse on the phone:** held keys walk and turn the camera, and a captured mouse turns it as on PC

### Hunting aids (all optional)

- **Damage numbers**, four styles, with **By hit**'s colours (weak points, hard parts, critical hits, the element), on
  small monsters too
- **The monsters' health bars**, with the game's names
- **The monsters always on the map**, as with a paintball
- **The bow shoots at the monster** the hunter faces

### Comforts (all optional)

- **Gather everything** a point has in one press, or **hold A** to gather again and again
- **Move to cancel the flex**, and **B cancels the sharpening**
- **The rewards screen with no time limit**, and **ZL+ZR ends the wait** after a quest
- **Send all to the box:** the rewards screen's **Send to Item Box** sends every reward at once
- **Faster eating and drinking:** MH3U's own cheats come with the app (off)
- **The Smithy's weapon tree:** while the Smithy's Forge or Upgrade list shows a weapon, a button (or the right stick)
  opens its tree with the game's icons: each weapon's attack, element or status (a hidden one with Awaken), sharpness,
  affinity, slots, rarity and what forging or upgrading it takes

### And more

- **Saves:** import from Azahar, Citra, Checkpoint, JKSM or any folder, export, a **Save folder** of your own (for a sync
  app), and a copy in `save_backups` of every save a change replaces
- **Event quests:** import them; in MH3U they are already every hunter's
- **Mods and cheats:** mods in the load order you choose, with their conflicts shown, and cheats in Azahar's format
- **Multiplayer:** hubs on the internet with no ports to open, rooms on your network and Azahar's, phones and PCs
  together, and the chat. See [Playing together](#playing-together)
- **Verify game files**, an error screen with **Share log**, the game waits while the settings are open (playing alone)
  and while the app is out of sight, a home screen shortcut, and a notice when a new version is out
- **The game's code built on the phone**, with no PC

What MH4U has and MH3U doesn't isn't shown: the insect glaive and the charge blade, the Palico's rewards, the port's kick
button, the supply box's wait and an anti-aliasing of the game's own.

## How well does it run?

Measured on the phone during development, at the phone's own settings unless it says otherwise:

| Device | GPU | RAM | Frames a second |
|---|---|---|---|
| Galaxy S25 Ultra | Adreno 830 | 12 GB | Low-end mode: 60 in Moga Village, the Moga Woods and Port Tanzia at a limit of 60; 120 in Port Tanzia at 120 |
| Anbernic RG406V | Mali-G57 | 8 GB | Low-end mode, limit 60: 60 in Moga Village and the Moga Woods, about 59 in Port Tanzia; 2x resolution: 60 in Port Tanzia |
| moto g06 power | Mali-G52 MC2 | 4 GB | Low-end mode, limit 60: about 56 to 57; 2x resolution: about 52 in Port Tanzia; Toaster and Brick: a steady 30 |

The game's own short stops (every 15 seconds or so, while it rebuilt its local session) are gone, and the renderer leaves
out what a hidden touch screen would draw: where the phone keeps up, MH3U runs as evenly as MH4U. **If your phone runs hot,
lower the frame rate limit**, or try Toaster mode.

## What you need

- **An Android device:** Android 9 or newer, 64-bit, with Vulkan. Tested on Mali and Adreno GPUs. A phone with a 32-bit Android (many Android Go phones) can't install the app.
- **About 3 GB free** on the device: the game takes about 2 GB, and the app needs about 1 GB more while it builds the game's
  code.
- **An internet connection** the first time the app builds the game's code (its tools, about 100 MB), and **time**: about
  an hour and a half on a low-end phone, once.
- **Your own copy of the game, decrypted:** Monster Hunter 3 Ultimate for the USA (title 00040000000AE400) as a `.3ds` or
  `.cci` file. It has no update: the game alone is enough. The European copy isn't supported yet.

This project does not explain how to dump a game from your console, and does not provide game files, keys or links to them.

## Setup

You do this once (and the third step again when a new version of the app asks for it).

### Step 1: Put your game in a folder

Put your game in a folder of its own on the phone or its SD card, for example `MH3U`, anywhere except inside
`Android/data`. Any file name works. Use your own copy: game files from other places are often encrypted, incomplete or
for another region, and then the app stops with an error. Once the game opens, gear, **System**, **Your game**, **Verify
game files** reads the game whole and tells you what is missing, cut short or changed.

If you also play MH4U, its folder can hold MH3U's game too: each app makes and reads its own game code.

### Step 2: Install the app

1. On your device, download the APK from [Releases](../../releases/latest) and open it. Android may ask you to allow
   installing apps from your browser.
2. Open **MH3U**, tap **Give permission and choose folder**, and turn on file access when Android asks. The game only reads
   that folder: nothing is copied or changed there.
3. Find your `MH3U` folder (it shows a ✓) and tap **Use this folder**. If you'd rather use Android's own picker, tap
   **Android's picker**.

MH3U is an app of its own (`com.mh3u.native`): it installs beside MH4U for Android and MHGU for Android, with its own saves
and settings, kept in `Android/data/com.mh3u.native/files`. To choose another folder later: gear, **System**, **Your
game**, **Game folder**.

### Step 3: Build the game's code

The app has no game code in it, so you build yours from your own copy, once. Nothing is sent anywhere.

The app says the library is missing: tap **Build on this phone**, then **Build**. Later it is also in the gear,
**System**, **Game code**, **Build the game code**.

- The first time it downloads its tools, about 100 MB: Python from python.org and Zig, a C compiler, from PyPI. Both are
  free software, checked before they are used, and nothing of the game is ever downloaded.
- It needs about 1 GB free while it works; when it ends, only the tools stay, and **Delete the tools** removes them too.
  On a phone with 4 GB of memory (RAM), close the other apps first.
- It took about 88 minutes on a moto g06 power (a low-end Mali-G52). It goes on with the screen off or in another app,
  with its progress in a notification, and if it is cut **Go on** goes on from where it stopped. **Start over** deletes
  what a cut, cancelled or failed build left, so the next **Build** starts from the beginning.
- The phone gets warm: keep it charging, out of its case if you can.

When it says the game code is ready, `libmh4u_code_native.so` and `libmh4u_code_native.id` are in your folder, and
**Play** starts the game. **These two files are made from your game: keep them for yourself and don't share them.**

Developers can build the same library on a PC from this repository's source: see [Building the app](#building-the-app).

## Update to a new version

Download the new APK from [Releases](../../releases/latest) and open it: it installs over the app you have. Your saves,
settings, layout and game folder stay. If the app then says the library is from another version, build it again (gear,
**System**, **Game code**, **Build the game code**). Each version's notes say whether the library stays valid.

## Settings

Tap the gear on the screen to open the settings. A long press on it opens the layout editor. The settings have six
sections, in the column at the left: **Controls**, **Graphics & Audio**, **Gameplay**, **Multiplayer**, **Mods & Cheats**
and **System**, with **Credits & Licenses** under them. A row with an arrow opens a page with the rest (**Screen and HUD**,
**Advanced**, **Join a room**...). The **?** at a row's right says what it does. Change what you like and tap **Save**; a
few options say they need a restart, which **Save and restart** does for you. Only what MH3U has is shown.

- **3D resolution:** 100% is your phone's own; lower is lighter on the phone. **1x** is the console's own, for the weakest
  phones. It can only be a whole number of times the console's screen (400x240), so each percentage goes to the nearest.
- **Frame rate limit:** 15 to 240. 30 is the console's, heats the phone less and saves battery; above 60 is for fast
  phones with a screen that fast.
- **Low-end mode:** **Off**, **On**, **Toaster** or **Brick**. You can still change each of its options afterwards.
- **Single-screen mode** (gear, **Graphics & Audio**, **Screen and HUD**, on unless you turn it off): see
  [What it does](#display-and-performance). The touch screen's button (or key) brings the touch screen back, with the
  menus on it, as on the console.
- **Hunting aids**, **HUD size**, **Text outline** and the **FPS counter** are in **Screen and HUD** too; the comforts,
  **Send all to the box** and the weapon tree in **Gameplay**.
- **Clear shader cache** (gear, **Graphics & Audio**, **Advanced**): the next start deletes the shaders the game compiled
  and compiles them again, for a picture that shows something wrong after a GPU driver update.

## Import your save

1. Copy your save to the device, for example into `Download`: from Azahar or Citra, the folder **Open Save Data Location**
   opens (right-click the game), or the emulator's whole user folder, the one with `sdmc` in it; or a Checkpoint or JKSM
   backup.
2. In the app: gear, **System**, **Your game**, **Import save**, **Choose a folder**, and pick it.

The save it replaces is kept in `save_backups`. **Export save** writes yours in the emulators' own layout (`MH3U save`).

**Save folder** (gear, **System**, **Your game**) keeps your save in a folder of your choice, where a sync app (FolderSync,
Autosync for Google Drive) can keep it the same as one in the cloud. If that folder already has a save, the app asks which
one goes on, and keeps the other in `save_backups`. Play on one device at a time, and save before you switch.

## Mods and cheats

Gear, **Mods & Cheats**. Mods for MH3U USA go in your game folder's `mods` (each with a `romfs` folder, an `exefs` folder
or both, or the emulators' layout with a `00040000000AE400` folder), in the load order you choose; mods that change the
same files are shown. Cheats are in Azahar's format (**Add cheats**). HD texture packs made for Azahar go in your game
folder's `textures`.

## Playing together

MH3U plays together in **Port Tanzia**: at Moga Village's pier, Neko's ferry takes you there. **Multi** opens a port of
your own for others to join; **Friend Search** finds the ports of the hunters in your hub or room. Then the Tavern's quest
board, as on the console.

- **Hubs: online with no ports to open.** Gear, **Multiplayer**, **Join a hub** lists the hubs on the community's server:
  join one, or **Create a hub** for your friends (a name, and a password if you like). Phones and PCs play in the same
  hubs. MH3U's hubs are its own: MH3U and MH4U players never meet in one.
- **Rooms on your network:** **Host a room**, and the others **Search this network** in **Join a room**. Rooms in Azahar's
  format work with Azahar too.
- **The chat:** a window over the game, with tabs, in rooms and hubs; on PC, **/** opens it.
- **Wait for a player who stalls** (gear, **Multiplayer**) holds the game a moment for a player whose connection is slow.
- **Leaving on a quest together:** at the Tavern's pier the host's A says **Depart on Quest** and the others' **Stand By
  for Departure**, in the same spot. If the host's A shows the other hunter's name instead (standing right behind them),
  take a step aside or under the arch until it says **Depart on Quest**: then **Confirm Departure** comes up.

Played during development: a phone and a PC in one hub and on one network, each of them the host, with quests together of
8 to 10 minutes and no one dropped; two phones in one hub, with a quest together; and a whole quest between a phone host
and a PC, single-screen mode on in both, from the departure to the rewards and back to the Tavern.

## If something goes wrong

| Problem | What to do |
|---|---|
| The app says the folder has no game, or another game | The folder needs Monster Hunter 3 Ultimate for the USA, decrypted. The European copy isn't supported yet |
| "App not installed" | A copy signed with another key is installed: export your save, uninstall that copy, install the official APK and import the save |
| The library is from another version | Build it again: gear, **System**, **Game code**, **Build the game code** |
| The game closes with an error | The error screen has **Share log**: attach the log to an [issue](../../issues) with your phone's model, or ask in the [Discord](https://discord.gg/sk5xEjEMwF). Don't attach game files |
| It runs slowly or the phone gets hot | **Low-end mode**, a lower **Frame rate limit** or **3D resolution** |

## PC (Windows)

The same game folder plays on a Windows PC: `MH3U.exe` runs your game natively, with the same runtime, settings and
features as the app, and what a PC adds: keyboard and mouse, any window size, high frame rates and Discord.

The download, `MH3U-PC-<version>.zip`, has the program only: the game code (`mh3u_code_native.dll` and
`mh3u_code_native.id`) is made once from your own copy, by the program itself ([PC step 3](#pc-step-3-build-the-game-code)).
Nothing else is needed: no Python or compiler of your own.

### What it does on PC

Everything the app has (above), with the PC's own:

- **Keyboard and mouse,** with MH4U for PC's keys: WASD moves, the mouse turns the camera, left click attacks, F is A,
  left Shift runs, left Ctrl walks, E sheathes and uses items, a tap on **Alt** locks on (Alt+Tab doesn't). Every key can
  be changed, up to three per button, mouse buttons too. **F2** lists your controls over the game. See
  [PC controls](#pc-controls)
- **Lock-on:** a tap on **Alt** (or a click of the right stick) and the camera keeps turning to the monster, also under
  water; **Tab** (ZL) steps the Target Cam through the large monsters
- **The Smithy's weapon tree:** **Alt** (or the right stick's click) while the Smithy's Forge or Upgrade list shows a weapon
- **Type with your keyboard:** your hunter's name and the game's other texts; in a room or hub, **/** opens the chat
- **The touch screen beside the game** (the PC's default), under it, in a corner or in a window of its own, with the mouse
  as your finger; or single-screen mode, as in the app
- **Any window:** a borderless full screen at your screen's resolution by default, a window of any size, or exclusive full
  screen; **F11** or **Alt+Enter** switches. The picture takes the window's shape at once, with **HUD anchor** for wide
  screens
- **Frame rate:** limits up to 240, or **No limit**, with **VSync** on or off; the game keeps its normal speed at any frame
  rate. **Show FPS and PC stats** shows the frame rate, and with **Full** the GPU's and the CPU's use
- **The picture options of the app,** with an **Upscale filter** for the scaled picture (Smooth, Sharp pixels, Nearest or
  FSR 1), **HDR output** on a screen with Windows' HDR on, and **GPU** on a PC with two graphics cards
- **A controller:** connect it and play. START and SELECT together open the settings; the menus can show your controller's
  buttons (**Button icons**)
- **Discord:** with the Discord app open on the same PC, your profile shows what you are doing: Moga Village or Port
  Tanzia, the quest you play, its monsters, the map and the area, your weapon, and your hunter's name and HR (once the
  Guild in Port Tanzia has registered you). **F1**, **Multiplayer**, **Discord** turns it off, or leaves out your name
- **Saves, mods, cheats, event quests, hubs and chat** as in the app, from the same **F1** menu. A PC and a phone play
  together in one hub or on one network
- If the game closes because of an error, a window says what happened and opens the log's folder; the game stops while its
  window is minimized (**F1**, **System**, **Pause in the background**)

### What you need on PC

- **Windows 10 or 11,** 64-bit.
- **A graphics card with Vulkan,** with its driver up to date.
- **8 GB of RAM** recommended.
- **Free space:** about 2 GB more than your game while the game code is made (about 300 MB stays, the tools, which can be
  deleted afterwards), and about 200 MB for the program and its caches.
- **An internet connection** the first time the game code is made: its tools take about 90 MB.
- **Your own copy of the game, decrypted:** Monster Hunter 3 Ultimate for the USA (title 00040000000AE400) as a `.3ds` or
  `.cci` file. It has no update: the game alone is enough.

### PC setup

You do this once.

#### PC step 1: Put your game in a folder

Your game in a folder of its own, for example `C:\MH3U`. If you also play MH4U on this PC, the same folder can hold both
games: each program makes and reads its own game code (`mh3u_code_native.dll`, `mh4u_code_native.dll`).

#### PC step 2: Install it

There is no installer: the program is a folder.

1. Unzip all of `MH3U-PC-<version>.zip` into a folder of your own, such as `C:\Games` (from inside the zip `MH3U.exe`
   doesn't start). Not into Program Files: the program keeps its settings, caches, log and the tools that make the game
   code in its own folder, and Windows doesn't let it write there.
2. Open `MH3U.exe`. Windows may say it doesn't know the program, because it isn't signed: **More info**, **Run anyway**.
3. The first time, a screen asks for your game folder: **Choose folder**, and pick it. The program checks it and says
   exactly what is missing if something is wrong. It remembers the folder.

#### PC step 3: Build the game code

The program has no game code in it, so it makes yours from your own copy, once, on your PC. Nothing is sent anywhere.

When your folder has the game but not the game code, the screen says that `mh3u_code_native.dll` is missing: press
**Build the game code on this PC**, then **Build**.

- The first time it downloads its tools, about 90 MB: Python from python.org and Zig, a C compiler, from PyPI, or from
  ziglang.org when PyPI can't be reached. Both are free software, checked against their SHA-256 before they are used, and
  nothing of the game is ever downloaded.
- It needs about 2 GB free on the disk where `MH3U.exe` is while it works. When it ends only the tools stay (about 300 MB,
  in the `builder` folder beside `MH3U.exe`), and **Delete the tools** removes them too.
- It takes about 10 to 40 minutes, depending on the PC. It uses all the processor's cores but one, at low priority, so you
  can use the PC meanwhile.
- **Cancel** stops it, and so does closing the window or restarting the PC: what it did is kept, and **Go on** continues
  from there. **Start over** forgets a build half done and starts from the beginning.

When it says the game code is ready, your folder has two new files, `mh3u_code_native.dll` and `mh3u_code_native.id`, and
**Play** starts the game. **These files are made from your game: keep them for yourself and don't share them.**

The zip holds one folder with the program's files and nothing of the game: `MH3U.exe` and `SDL3.dll`; `zigcc.exe` and
`builder_tools.zip`, what the builder uses; `code_library_id.txt`, the id of the game code this version needs (your
folder's `mh3u_code_native.id` must say the same); `cheats`, the app's own optional cheats; `licenses`; and `README.txt`, a
short guide.

### PC controls

- **Keyboard and mouse** (the same keys as MH4U and MHGU for PC): WASD moves, left Ctrl walks, I, J, K, L turn the
  camera. Left click or R is X; F or the right click is A (the right click only with the weapon out, in a menu or a talk);
  Space is B; E or the middle button is Y (sheathe, use an item); Q is L, left Shift is R (run, guard, aim); Tab and T are
  ZL and ZR; Enter is START, Backspace SELECT, and the arrows are the D-pad. M (or right Ctrl) shows or hides the touch
  screen. A tap on Alt locks onto the monster (Alt+Tab doesn't). The mouse wheel steps the item bar. Click the window to
  turn the camera with the mouse; Esc lets the mouse go.
- **The program's own keys:** **F1** the settings, **F2** the list of the controls, **F11** or **Alt+Enter** full screen
  or a window, **1** the Item Pouch panel of a quest's touch screen, **G** the town's Gestures, and **/** the chat in a room
  or hub.
- **Every key can be changed,** up to three per button, mouse buttons included: **F1**, **Controls**, **Keyboard and
  mouse**, **Keybinds**. The mouse's axes can be inverted there too. If you had changed no key, the keys of an earlier
  build become these by themselves; one key of your own, and nothing is changed.
- **The touch screen:** a click on it is a tap.
- **The settings menu (F1)** works with the mouse, the keyboard (the arrows, Enter, Esc, Tab and Page Up/Down change the
  section, Ctrl+S saves) or a controller.
- **Controller:** connect it and play. START and SELECT together open the settings. The right stick turns the camera; in
  the towns, where the game's camera is fixed, it is the Circle Pad Pro's, as on the console. If the game doesn't read your
  controller well: **Controls**, **Controller**, **Map a controller**.

### PC saves

Your save is in `%APPDATA%\MH3U` (type it in File Explorer's address bar), apart from MH4U for PC's, so a new version
unzipped elsewhere finds it. **F1**, **System**, **Your game**: **Import save**, **Export save** and **Save folder**
(nothing is overwritten without asking: the save that doesn't go on is kept in `%APPDATA%\MH3U\save_backups`).

### Where things are kept on PC

| What | Where |
|---|---|
| Settings, the folder you chose, the log | Beside `MH3U.exe`: `mh4u-app.ini`, `game_folder.txt`, `log.txt` (the sessions before: `log_prev.txt`, `log_prev2.txt`) |
| The game's small files, the game's own settings, cheats, mods | Beside `MH3U.exe`: `game`, `user` (`mh4u.ini`, `cheats`, `load/mods`) |
| The shader cache | `%LOCALAPPDATA%\MH3U\cache` |
| The tools that make the game code, and their log | Beside `MH3U.exe`: `builder`, `builder_log.txt` |
| Your save | `%APPDATA%\MH3U` (or the folder you chose in **Save folder**) |
| Copies of the saves a change replaced | `%APPDATA%\MH3U\save_backups` |
| Your game, the game code, mods and HD packs | Your game folder: `mh3u_code_native.dll` and `.id`, `mods`, `textures` |

### Update or uninstall the PC version

**Update:** unzip the new version into a new folder and open its `MH3U.exe`; your save is in `%APPDATA%\MH3U` and is found
by itself. If it says the game code is from another version, press **Build the game code on this PC**.

**Uninstall:** delete the program's folder, the one with `MH3U.exe`. Your save stays in `%APPDATA%\MH3U`, and the shader
cache in `%LOCALAPPDATA%\MH3U`: delete those too only if you want them gone (keep a copy of the save first). Nothing else
is installed.

### If something goes wrong on PC

| Problem | What to do |
|---|---|
| "SDL3.dll was not found" | Unzip the whole zip into a folder first: `MH3U.exe` doesn't start from inside the zip |
| "No game here ... The game in it is another one" | The folder has another game (MH4U, or another region's MH3U): this program runs Monster Hunter 3 Ultimate for the USA |
| "The game's code library (mh3u_code_native.dll) isn't in the folder" | The download doesn't include it: press **Build the game code on this PC** on that screen |
| "This mh3u_code_native.dll is for another version" | It was made for another version: press **Build the game code on this PC**, and it is made again for this one |
| It closes at the start, or the window stays black | Update your graphics card's driver |
| The mouse doesn't turn the camera | Click the window first. Esc lets the mouse go |
| Nobody can join the hub you host | Windows Firewall asks the first time whether MH3U may use the network: allow it |

If the game closes because of an error, a window says so; **Open the log folder** opens the folder with `log.txt`. Attach it
to an [issue](../../issues) with your PC's processor and graphics card, or ask in the
[Discord](https://discord.gg/sk5xEjEMwF). Don't attach game files.

## Building the app

For developers; players only need the APK or the PC zip from Releases. On Windows (Git Bash), Linux or macOS, with the
Android SDK (platform 35) and NDK 27.0.12077973, CMake 3.25 or newer, Ninja, Python 3.10 or newer and a JDK 17:

```
sh scripts/build_sdl3.sh    # SDL 3.4.18 from its release, once
sh scripts/build_apk.sh     # libmain.so, the hook libraries, the on-phone builder's launchers and tools, then Gradle
```

`scripts/android_env.sh` says where each tool is looked for. Sign with your own key in `android/keystore.properties` and
`android/mh3u.keystore`; without them you get a debug APK. `app_native/` is the app (SDL3 and Dear ImGui), `android/` its
Android side, `app_native/pc/` the PC's, and `mh3n/` the runtime and the recompiler (see [mh3n/README.md](mh3n/README.md)).

The PC version, in Git Bash on Windows, with [llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) 20241217 for
x86_64 with the UCRT, CMake 3.25 or newer, Ninja and Python 3.10 or newer:

```
LLVM_MINGW=C:/llvm-mingw-20241217-ucrt-x86_64 bash scripts/build_pc.sh            # build/pc/MH3U, as a player unpacks it
LLVM_MINGW=C:/llvm-mingw-20241217-ucrt-x86_64 PACK=1 bash scripts/build_pc.sh     # and the release's zip with its .sha256
```

It builds SDL 3.4.18 for Windows from its release the first time.

The game's code library for the app can also be made on a computer, from your own copy (Python 3.10 or newer; on Windows
`--fetch-compiler` downloads llvm-mingw, elsewhere clang or Zig's C compiler is used):

```
python mh3n/tools/build_game.py --dump <your game .cci> --native --android --fetch-compiler --out build/game
```

It writes `libmh4u_code_native.so` and its `.id` under `build/game/recomp/android/`: copy both into your game folder on
the phone.

## Legal

This project is not affiliated with, endorsed by or sponsored by the Great C or the Great N. Monster Hunter is a trademark of
the Great C.

This repository and its releases contain no game code, game data, ROMs or keys. Use it only with a copy of the game you own.
Requests for game files, keys or ready-made `libmh4u_code_native.so` or `mh3u_code_native.dll` files will be closed.

Rights holders: please open an issue to get in touch.

## Credits

- The game's recompiler and the native runtime come from [MH4UltimateDC](https://github.com/CompaLarios18/MH4UltimateDC)
  by CompaLarios18 and its contributors.
- Parts of the runtime are derived from [Azahar](https://github.com/azahar-emu/azahar) and Citra.
- MH3U for Android and PC is made from [MH4U for Android and PC](https://github.com/khorne-hub/4u-native), by the same
  author; from the same author too, [MHGU for Android and PC](https://github.com/khorne-hub/gu-native). Parts of the PC
  version (its stats readout, its zip reader and Discord Rich Presence) follow MHGU for PC's.
- Contributed to development:
  - tabarong
  - Klark231
  - Alfonso Matador
  - popzol
  - BlitzWOLF
  - jM5557
- The app's builder, which makes the game's code on the phone, downloads Python ([python.org](https://www.python.org), PSF
  License) and [Zig](https://ziglang.org) (MIT; with LLVM, Apache 2.0 with LLVM exceptions) the first time it runs; their
  licenses are in the app's **Licenses** too.
- The libraries this project uses, with their licenses, are listed in [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES). Their
  licenses are also in the app: gear, **Credits & Licenses**, **Licenses**.

## Official downloads

MH3U for Android and PC is free. The only official downloads are the links khorne-hub gives, starting with this repository's
[Releases](https://github.com/khorne-hub/3u-native/releases). If you got it anywhere else, paid for it, or your copy came with
ads, it is not the official one. In the app, the GitHub button at the top of the settings menu, beside **Save**, opens this
page in your browser, and the Discord button beside it the community's [Discord](https://discord.gg/sk5xEjEMwF).

Every official APK is signed with the same key, MH3U's own. Its certificate's SHA-256 fingerprint is:

```
83:7B:CE:4A:04:04:FB:5D:F2:BC:C0:46:BE:91:0A:D0:1F:87:E6:52:98:BF:75:5E:87:55:9C:A2:EC:F6:BE:D3
```

To check an APK: `apksigner verify --print-certs MH3U-<version>.apk` (Android SDK build-tools). A copy signed with another
key can't be installed over the official app, and the app says so once when it starts.

The PC version is the zip `MH3U-PC-<version>.zip`, with its SHA-256 beside it in `MH3U-PC-<version>.zip.sha256`. To check
yours, in a Command Prompt: `certutil -hashfile MH3U-PC-<version>.zip SHA256`, and compare it with that file.

The app also tells you when a new version is out. At most once a day it asks GitHub for this repository's latest release
(nothing else is sent, not even the phone's model), and at the next start it offers to open the release page. It never
downloads or installs anything by itself. Gear, **System**, **Check for updates** turns it off.

## License

MH3U for Android and PC is free software under the [GNU General Public License, version 3](LICENSE) or (at your option) any
later version, like the MH4UltimateDC runtime it builds on. The third-party parts keep their own licenses.

The code by khorne-hub carries [additional terms](ADDITIONAL-TERMS), which section 7 of the GPL allows. Whoever shares the
app, changed or not, must:

- keep the author's credits: the ones the app shows when it opens, its **Credits & Licenses** page, and this file's
  **Credits** and **License** sections;
- mark a changed version as changed, and not present it as the official one;
- give, with the app, its license and its complete source code, as the GPL requires.
