// Key routing decisions for the NSEvent monitor, as plain C++ so they can be unit tested on any machine.
// Key codes are macOS virtual key codes (Carbon kVK_* values).
#pragma once

#include <string>

namespace wvp::mac {

enum class Route {
  Action,   // a shortcut sent as an AppKit action inside the web view (copy:, selectAll:, ...); key up is
            // swallowed too
  Deliver,  // delivered to the first responder with keyDown: / keyUp: (text input, IME, arrows), and consumed
  Pass      // not ours: returned to the event stream for After Effects (Cmd+S, Cmd+Q, Ctrl combinations, ...)
};

enum class Action {
  None,
  Copy,
  Paste,
  Cut,
  SelectAll,
  Undo,
  Redo,
  LineStart,
  LineStartSelect,
  LineEnd,
  LineEndSelect,
  PageUp,
  PageUpSelect,
  PageDown,
  PageDownSelect
};

struct KeyInput {
  unsigned short keyCode = 0;
  bool command = false;
  bool control = false;
  bool option = false;
  bool shift = false;
  char base =
      0;  // the key without modifiers, lower case, when it is plain ASCII (charactersIgnoringModifiers)
};

struct KeyDecision {
  Route route = Route::Pass;
  Action action = Action::None;  // only for Route::Action
};

KeyDecision classifyKey(const KeyInput& key);

// The NSResponder selector name for an action ("copy:", "moveToBeginningOfLine:", ...).
std::string actionSelector(Action action);

// The page JavaScript that does the same when nothing inside the web view answers the selector; "" when there
// is none.
std::string actionJsFallback(Action action);

// The identity of one key event, enough to recognise WebKit handing back an event the monitor already routed.
struct KeyStamp {
  double timestamp = 0;
  unsigned short keyCode = 0;
  unsigned long type = 0;
  long windowNumber = 0;
  bool valid = false;
};

// True when `now` is the same physical event as `routed` (same type, key code, window and timestamp).
bool isResendOf(const KeyStamp& routed, const KeyStamp& now);

}  // namespace wvp::mac
