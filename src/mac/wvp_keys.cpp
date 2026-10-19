// Key routing rules. Editing shortcuts and Home/End/Page keys are actions, Cmd+arrow and Cmd+Backspace are
// text editing, other Cmd and Ctrl combinations belong to AE, and plain keys go to the page.
#include "wvp_keys.h"

#include <cmath>

namespace wvp::mac {

namespace {

enum : unsigned short {
  kKeyLeft = 123,
  kKeyRight = 124,
  kKeyDown = 125,
  kKeyUp = 126,
  kKeyBackspace = 51,
  kKeyForwardDelete = 117,
  kKeyHome = 115,
  kKeyEnd = 119,
  kKeyPageUp = 116,
  kKeyPageDown = 121
};

constexpr double kSameInstantSeconds = 1e-6;

bool isNavigationOrDelete(unsigned short code) {
  return code == kKeyLeft || code == kKeyRight || code == kKeyDown || code == kKeyUp ||
         code == kKeyBackspace || code == kKeyForwardDelete;
}

Action editingKeyAction(unsigned short code, bool shift) {
  switch (code) {
    case kKeyHome:
      return shift ? Action::LineStartSelect : Action::LineStart;
    case kKeyEnd:
      return shift ? Action::LineEndSelect : Action::LineEnd;
    case kKeyPageUp:
      return shift ? Action::PageUpSelect : Action::PageUp;
    case kKeyPageDown:
      return shift ? Action::PageDownSelect : Action::PageDown;
    default:
      return Action::None;
  }
}

Action commandShortcut(char base, bool shift) {
  if (shift) return base == 'z' ? Action::Redo : Action::None;
  switch (base) {
    case 'c':
      return Action::Copy;
    case 'v':
      return Action::Paste;
    case 'x':
      return Action::Cut;
    case 'a':
      return Action::SelectAll;
    case 'z':
      return Action::Undo;
    default:
      return Action::None;
  }
}

KeyDecision decision(Route route, Action action = Action::None) { return KeyDecision{route, action}; }

}

KeyDecision classifyKey(const KeyInput& key) {
  if (!key.command && !key.control) {
    const Action editing = key.option ? Action::None : editingKeyAction(key.keyCode, key.shift);
    return editing != Action::None ? decision(Route::Action, editing) : decision(Route::Deliver);
  }
  if (!key.command || key.control || key.option) return decision(Route::Pass);
  const Action shortcut = commandShortcut(key.base, key.shift);
  if (shortcut != Action::None) return decision(Route::Action, shortcut);
  return decision(isNavigationOrDelete(key.keyCode) ? Route::Deliver : Route::Pass);
}

namespace {

struct ActionInfo {
  Action action;
  const char* selector;
  const char* script;
};

constexpr ActionInfo kActions[] = {
    {Action::Copy, "copy:", "document.execCommand('copy')"},
    {Action::Paste, "paste:", ""},
    {Action::Cut, "cut:", "document.execCommand('cut')"},
    {Action::SelectAll, "selectAll:", "document.execCommand('selectAll')"},
    {Action::Undo, "undo:", "document.execCommand('undo')"},
    {Action::Redo, "redo:", "document.execCommand('redo')"},
    {Action::LineStart,
     "moveToBeginningOfLine:", "window.getSelection().modify('move','backward','lineboundary')"},
    {Action::LineStartSelect, "moveToBeginningOfLineAndModifySelection:",
     "window.getSelection().modify('extend','backward','lineboundary')"},
    {Action::LineEnd, "moveToEndOfLine:", "window.getSelection().modify('move','forward','lineboundary')"},
    {Action::LineEndSelect,
     "moveToEndOfLineAndModifySelection:", "window.getSelection().modify('extend','forward','lineboundary')"},
    {Action::PageUp, "scrollPageUp:", ""},
    {Action::PageUpSelect, "pageUpAndModifySelection:", ""},
    {Action::PageDown, "scrollPageDown:", ""},
    {Action::PageDownSelect, "pageDownAndModifySelection:", ""},
};

const ActionInfo* infoFor(Action action) {
  for (const ActionInfo& info : kActions)
    if (info.action == action) return &info;
  return nullptr;
}

}

std::string actionSelector(Action action) {
  const ActionInfo* info = infoFor(action);
  return info != nullptr ? info->selector : "";
}

std::string actionJsFallback(Action action) {
  const ActionInfo* info = infoFor(action);
  return info != nullptr ? info->script : "";
}

bool isResendOf(const KeyStamp& routed, const KeyStamp& now) {
  return routed.valid && now.valid && routed.type == now.type && routed.keyCode == now.keyCode &&
         routed.windowNumber == now.windowNumber &&
         std::fabs(routed.timestamp - now.timestamp) < kSameInstantSeconds;
}

}
