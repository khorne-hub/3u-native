// The game's own software keyboard, typed on from outside (docs/research/keyboard.md). MH4U does not use the system's
// keyboard applet: it carries its own copy of it (`sSoftwareKeyboard`, the `nSwkbd` classes), drawn on the touch
// screen, for the Guild Card's greeting, the names, the hall's password and the like. The app shows the phone's own
// keyboard instead while the game's is open, and its text goes into the game's text box (where the game's own keys
// keep working, and what they type comes back here); OK and Cancel are the game's own buttons, pressed for the player,
// so the game checks the text as it checks its own keys'.
//
// Frame runs on the emulation thread, once a frame (hooks.cpp's sMhMain::frame, before the game's frame): it reads the
// keyboard's state and carries out what the app asked for since. Everything else may be called from any thread.
// Nothing here logs the text.
//
// A game that uses the system's keyboard applet instead (MH3U: the hunter's name, the chat) reaches the same functions
// through AppletFrontend, the runtime's keyboard frontend (mh4n/applet_frontend.h): the applet's request is the open
// keyboard (Request::applet), SetText only keeps the text, and Submit and Cancel answer the applet with its OK and
// Cancel buttons. No hook and no address of the game's: it serves any game that opens the applet.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#if __has_include("mh4n/applet_frontend.h")
#include "mh4n/applet_frontend.h"
#endif

struct RCtx;

namespace Port::Keyboard {

/// What the game's keyboard asks for, while it is open.
struct Request {
    bool open = false;
    std::uint32_t serial = 0;       ///< one per keyboard the game opens
    bool numeric = false;           ///< the number pad (the game's keyboard type 2)
    std::uint32_t max_length = 0;   ///< UTF-16 units
    /// The game's buttons: 0 OK alone, 1 Cancel and OK, 2 Cancel, I Forgot and OK, 3 none.
    std::uint32_t buttons = 0;
    /// The text the game takes: 0 anything, 1 not empty, 2 not empty and not blank, 3 not blank, 4 exactly
    /// max_length characters.
    std::uint32_t accept = 0;
    std::uint32_t filters = 0;      ///< 1 at most max_digits digits, 2 no '@', 4 no '%', 8 no '\'
    std::uint32_t max_digits = 0;
    std::u16string hint;            ///< the game's hint for the empty field (often none)
    std::u16string text;            ///< what the game's text box holds now
    std::uint32_t text_serial = 0;  ///< changes when the game changes `text` (its own keys), not for SetText's
    /// The system's keyboard applet, not the game's own keyboard: there are no game's keys to type with, only the
    /// app's (the text the game starts it with is `text`).
    bool applet = false;
};

/// The keyboard open now (a copy).
Request Current();

/// The game's text box takes `text` (cut to the keyboard's length) at the next frame, the cursor at its end. Ignored
/// unless `serial` is the open keyboard's.
void SetText(std::uint32_t serial, const std::u16string& text);
/// `text`, then the game's OK button.
void Submit(std::uint32_t serial, const std::u16string& text);
/// The game's Cancel button (a keyboard with one only).
void Cancel(std::uint32_t serial);

/// hooks.cpp: before each of the game's frames. Returns a line for the log, or "".
std::string Frame(RCtx* ctx);

#if __has_include("mh4n/applet_frontend.h")
/// The runtime's keyboard frontend that shows the system's keyboard applet through these functions (the app's keyboard
/// panel); set it with mh4n::SetKeyboardFrontend before the runtime boots.
std::shared_ptr<mh4n::KeyboardFrontend> AppletFrontend(mh4n::Runtime& runtime);
#endif

}  // namespace Port::Keyboard
