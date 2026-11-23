// MH3U's own versions of the port's gameplay features (mh3u_qol.cpp): the camera turned by the mouse and the lock-on
// (the camera's turn hook), the end of the wait after a quest, the rewards screen's comforts (no time limit, Send to
// Item Box sends all), gathering (everything in one press, hold A to gather again) and the readings for the PC app's
// Discord Rich Presence, the flex's and the sharpening's cancel and a bowgun's aim turned by the app's camera turn (the
// gyro, a drag, the mouse). MH4U's versions of these read MH4U's objects (hooks.cpp, gather.cpp, presence.cpp) and stay off
// on an MH3U library; these run only on one (hooks.cpp's InstallMh3u installs them), so MH4U's behaviour doesn't change.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct RCtx;
struct RCodeInfo;

namespace Port {
class Platform;
}

namespace Port::Mh3uQol {

/// A new boot: every option off and nothing remembered (hooks.cpp's LoadOptions, before mh4u.ini is read).
void Reset();

/// Takes the options this module reads (the same names as MH4U's: qol_hold_gather, qol_flex_cancel,
/// qol_sharpen_cancel, qol_rewards_no_limit, rewards_send_all, gather_all, skip_end, skip_end_hold, and its own traces
/// mh3u_trace and mh3u_camera_trace, and the research ones mh3u_aim_trace, mh3u_eat_trace, mh3u_aim_log, mh3u_aim_sign).
/// Returns true only for its own keys (the shared ones go on to hooks.cpp's options
/// too). Any thread before the game starts; the emulation thread after.
bool SetOption(const std::string& key, const std::string& value);

/// hooks.cpp's InstallMh3u: this module's hook points, found by name in the library (config/hooks.txt), its lines for
/// the start's log in `log`, the platform (the game's clock, the votes to end the wait) and where its later lines go.
void Install(const RCodeInfo& info, std::vector<std::string>& log, Platform* platform,
             void (*log_line)(const std::string& line));

/// Whether Install found this game's hook points (an MH3U library): the features run.
bool Active();

/// Once a game frame, from hooks.cpp's MH3U frame point (sMain::updateFrameTiming), before the game's frame.
void Frame(RCtx* ctx);

/// Which of the app's options do something on MH3U now (the settings hide the rest): a key of mh4u.ini or of the app's
/// own settings ("lock_on", "gyro_aim", "supply_box_wait"...). True for every key on an MH4U library and before a game.
bool Supports(const std::string& key);

}  // namespace Port::Mh3uQol
