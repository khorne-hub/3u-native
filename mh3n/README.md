# mh3n: the runtime and the game's recompiler

This folder comes from the `native` branch of [MH4UltimateDC](https://github.com/CompaLarios18/MH4UltimateDC), by
CompaLarios18 and its contributors, under the GPL-3.0-or-later ([LICENSE](LICENSE)), through MH4U for Android and PC
([4u-native](https://github.com/khorne-hub/4u-native)). MH3U for Android and PC builds on it, with its own changes for
MH3U (its function table, hook points and port code), under the same licence.

| Path | What |
|---|---|
| `tools/` | The recompiler (`recompile.py`, `recomp/`), `function_table.py` (MH3U's function table from its own code) and `build_game.py`, which turns the player's own copy of the game into the game's code library. `tools/android/phone_build.py` is the app's on-phone builder; `tools/pc_build.py` the PC program's |
| `config/` | The tables the recompiler and the runtime read: the game's functions, hook points, tunables and the port's addresses (`port_map.txt`), the versions it knows. They hold addresses, not game code |
| `native/` | The runtime: the console's kernel and services, the sound (DSP), the PICA200 GPU on Vulkan, mods, cheats and network rooms. `native/THIRD_PARTY.md` lists every vendored library and every file derived from Azahar, with its licence |
| `port/src/` | The game's hooks (widescreen, camera, frame pacing, HUD, controls, MH3U's own: `mh3u_*.cpp`) and the code host the recompiled code runs on |
| `port/app/` | Shared app sources (settings, input, rooms) and Dear ImGui |

Left out of this copy: the upstream's research and process documents, its CI, the Python tools' tests, its servers, the
patch of its Azahar-based PC version, and MH4U's PC scripts for the Android library (they ask for MH4U's update). They are
in the upstream repository.

The code library `build_game.py` makes is built from the player's own copy of the game: it is never part of this
repository or of the app, and it must not be shared.
