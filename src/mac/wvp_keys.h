// Key routing decisions for the event monitor, in plain C++ so they can be unit tested anywhere. Key codes
// are macOS virtual key codes (kVK_*).
#pragma once

#include <string>

namespace wvp::mac {

enum class Route { Action, Deliver, Pass };

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
  char base = 0;
};

struct KeyDecision {
  Route route = Route::Pass;
  Action action = Action::None;
};

KeyDecision classifyKey(const KeyInput& key);

std::string actionSelector(Action action);

std::string actionJsFallback(Action action);

struct KeyStamp {
  double timestamp = 0;
  unsigned short keyCode = 0;
  unsigned long type = 0;
  long windowNumber = 0;
  bool valid = false;
};

bool isResendOf(const KeyStamp& routed, const KeyStamp& now);

}
