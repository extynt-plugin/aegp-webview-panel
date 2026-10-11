// The event monitor: one NSEvent local monitor for the whole process.
//
// Why it exists. After Effects takes keys before a web view in a panel sees them: Cmd+A selects all layers,
// the arrows move the time cursor or steal focus, Cmd+C/V/X/Z do nothing in the page, and Delete edits the
// project. A local monitor runs inside NSApplication's event dispatch, before the event reaches a window, a
// menu key equivalent or a view, so it can route the key itself:
//   - the first responder is NOT inside one of our web views: the event is returned untouched (After
//   Effects').
//   - it is inside one: editing shortcuts are sent as actions to the responder inside the web view (never to
//   the key
//     window's whole chain, which is After Effects'); every other plain key is delivered with keyDown:/keyUp:
//     so text input and IME still work; the event is consumed, so After Effects never sees it. Cmd/Ctrl
//     combinations that are not editing (Cmd+S, Cmd+Q, ...) are returned for After Effects. The decision is
//     wvp_keys.cpp.
//   - a left mouse down on a web view makes its window key and the web view first responder (a floating panel
//   is not
//     key until clicked, and without this it takes no clicks or keys); a click elsewhere in a window whose
//     first responder is still inside a web view hands focus back, so Delete and arrows reach After Effects
//     again.
#import "wvp_mac.h"

#include <array>
#include <cstddef>

#include "wvp_keys.h"
#include "wvp_log.h"

namespace wvp::mac {

namespace {

id g_monitor = nil;
__weak NSResponder* g_previousResponder =
    nil;  // After Effects' first responder before a click focused a web view

// WebKit hands back a key it did not use by sending the same event through [NSApp sendEvent:] again, so the
// monitor sees it a second time; After Effects must not. The last few routed events are remembered (a short
// ring, because a quick key-up can overtake the key-down's reply).
constexpr std::size_t kRoutedRing = 4;
std::array<KeyStamp, kRoutedRing> g_routed;
std::size_t g_routedNext = 0;

KeyStamp stampOf(NSEvent* event) {
  KeyStamp stamp;
  stamp.timestamp = event.timestamp;
  stamp.keyCode = event.keyCode;
  stamp.type = event.type;
  stamp.windowNumber = event.windowNumber;
  stamp.valid = true;
  return stamp;
}

bool isResend(NSEvent* event) {
  const KeyStamp now = stampOf(event);
  for (const KeyStamp& routed : g_routed)
    if (isResendOf(routed, now)) return true;
  return false;
}

void rememberRouted(NSEvent* event) {
  g_routed[g_routedNext] = stampOf(event);
  g_routedNext = (g_routedNext + 1) % kRoutedRing;
}

WvpWebView* webViewContaining(NSResponder* responder) {
  if (![responder isKindOfClass:[NSView class]]) return nil;
  for (WvpWebView* web in liveViews().allObjects)
    if (responder == web || [static_cast<NSView*>(responder) isDescendantOf:web]) return web;
  return nil;
}

KeyInput inputOf(NSEvent* event) {
  const NSEventModifierFlags flags = event.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask;
  KeyInput key;
  key.keyCode = event.keyCode;
  key.command = (flags & NSEventModifierFlagCommand) != 0;
  key.control = (flags & NSEventModifierFlagControl) != 0;
  key.option = (flags & NSEventModifierFlagOption) != 0;
  key.shift = (flags & NSEventModifierFlagShift) != 0;
  NSString* base = [event.charactersIgnoringModifiers lowercaseString];
  if (base.length == 1 && [base characterAtIndex:0] < 128)
    key.base = static_cast<char>([base characterAtIndex:0]);
  return key;
}

void logKey(NSEvent* event, NSResponder* responder, const std::string& what) {
  if (!g_logKeys.load()) return;
  logLine(std::string("key ") + (event.type == NSEventTypeKeyDown ? "down" : "up") + " code=" +
          std::to_string(event.keyCode) + " firstResponder=" + className(responder) + " -> " + what);
}

// Sends the action to the first responder from `responder` up to the web view that answers it. When nothing
// inside answers, runs the JavaScript equivalent in the page. (The web view's undoManager is not used: it may
// be the window's, and After Effects' own undo stack must never be touched.)
std::string performAction(WvpWebView* web, NSResponder* responder, Action action) {
  const SEL selector = NSSelectorFromString(nsString(actionSelector(action)));
  for (NSResponder* candidate = responder; candidate; candidate = candidate.nextResponder) {
    if ([candidate respondsToSelector:selector]) {
      [NSApp sendAction:selector to:candidate from:web];
      return "action " + actionSelector(action) + " sent to " + className(candidate);
    }
    if (candidate == web) break;
  }
  const std::string script = actionJsFallback(action);
  if (script.empty()) return "action " + actionSelector(action) + ": nothing inside answers, swallowed";
  [web evaluateJavaScript:nsString(script) completionHandler:nil];
  return "action " + actionSelector(action) + ": page JavaScript fallback";
}

// An action is performed on key down and its key-up is swallowed too, so After Effects never sees half a
// keystroke; every other routed key is delivered to the responder as keyDown:/keyUp:.
NSEvent* deliver(NSEvent* event, WvpWebView* web, const KeyDecision& decision) {
  NSResponder* responder = event.window.firstResponder;
  const bool down = event.type == NSEventTypeKeyDown;
  if (decision.route == Route::Action) {
    logKey(event, responder, down ? performAction(web, responder, decision.action) : "key up swallowed");
    return nil;
  }
  logKey(event, responder, "delivered");
  rememberRouted(event);  // before delivery: WebKit's reply comes back asynchronously
  if (down) [responder keyDown:event];
  else [responder keyUp:event];
  return nil;
}

NSEvent* handleKey(NSEvent* event) {
  NSResponder* responder = event.window.firstResponder;
  WvpWebView* web = webViewContaining(responder);
  if (!web) return event;  // not ours: a key for the rest of After Effects
  if (isResend(event)) {
    logKey(event, responder, "WebKit's re-sent copy swallowed");
    return nil;
  }
  const KeyDecision decision = classifyKey(inputOf(event));
  if (decision.route == Route::Pass) {
    logKey(event, responder, "passed to After Effects");
    return event;
  }
  return deliver(event, web, decision);
}

// The web view that a click in `window` lands on, if any.
WvpWebView* webViewHit(NSWindow* window, NSEvent* event) {
  for (WvpWebView* web in liveViews().allObjects) {
    if (web.window != window || !web.superview) continue;
    NSView* hit = [web hitTest:[web.superview convertPoint:event.locationInWindow fromView:nil]];
    if (hit && (hit == web || [hit isDescendantOf:web])) return web;
  }
  return nil;
}

// A click elsewhere in a window whose first responder is still inside a web view (a docked panel shares After
// Effects' main window): hand focus back before After Effects sees the click.
void handFocusBack(NSWindow* window) {
  if (!webViewContaining(window.firstResponder)) return;
  NSResponder* previous = g_previousResponder;
  const bool restorable = previous && [previous isKindOfClass:[NSView class]] &&
                          static_cast<NSView*>(previous).window == window && !webViewContaining(previous);
  [window makeFirstResponder:restorable ? previous : nil];
}

void focusWebView(NSWindow* window, WvpWebView* web) {
  if (!window.keyWindow && window.canBecomeKeyWindow) [window makeKeyWindow];
  if (![web firstResponderIsInside]) {
    g_previousResponder = window.firstResponder;  // restored on the next click outside the web view
    [window makeFirstResponder:web];
  }
}

NSEvent* handleMouseDown(NSEvent* event) {
  NSWindow* window = event.window;
  if (!window) return event;
  WvpWebView* web = webViewHit(window, event);
  if (web) focusWebView(window, web);
  else handFocusBack(window);
  return event;  // never consumed: the click still reaches the page (or After Effects)
}

NSEvent* handleEvent(NSEvent* event) {
  if (liveViews().count == 0) return event;  // no panel open: nothing to route
  try {
    if (event.type == NSEventTypeLeftMouseDown) return handleMouseDown(event);
    if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp) return handleKey(event);
  } catch (...) {
    logDroppedException("event monitor");  // the event is passed on untouched
  }
  return event;
}

}  // namespace

void installMonitor() {
  if (g_monitor) return;
  const NSEventMask mask = NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskLeftMouseDown;
  g_monitor =
      [NSEvent addLocalMonitorForEventsMatchingMask:mask
                                            handler:^NSEvent*(NSEvent* event) { return handleEvent(event); }];
  logLine(g_monitor ? "event monitor installed" : "event monitor could not be installed");
}

void removeMonitor() {
  if (!g_monitor) return;
  [NSEvent removeMonitor:g_monitor];
  g_monitor = nil;
}

}  // namespace wvp::mac
