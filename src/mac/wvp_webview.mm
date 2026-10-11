// WvpWebView and the small helpers shared by the macOS files.
#import "wvp_mac.h"

#include <algorithm>

#include "wvp_platform.h"

namespace wvp::mac {

std::atomic<bool> g_logKeys{false};

NSHashTable* liveViews() {
  static NSHashTable* table = [NSHashTable weakObjectsHashTable];
  return table;
}

std::string stdString(NSString* text) { return text ? std::string(text.UTF8String) : std::string(); }
NSString* nsString(const std::string& text) { return [NSString stringWithUTF8String:text.c_str()]; }
std::string className(id object) { return object ? stdString(NSStringFromClass([object class])) : "nil"; }

}  // namespace wvp::mac

namespace {

// A floating panel window has no minimum size and can be dragged to a sliver. The window of a docked panel is
// After Effects' main window and is left alone. (After Effects may still override this for a floating window:
// Mac test.)
void limitFloatingWindow(NSWindow* window) {
  if (window == NSApp.mainWindow) return;
  const NSSize now = window.contentMinSize;
  window.contentMinSize = NSSize{std::max(now.width, static_cast<CGFloat>(wvp::platform::kMinimumWidth)),
                                 std::max(now.height, static_cast<CGFloat>(wvp::platform::kMinimumHeight))};
}

}  // namespace

@implementation WvpWebView

// A click that activates a floating panel is also delivered to the page (the window is not key until
// clicked).
- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}

- (BOOL)firstResponderIsInside {
  NSResponder* responder = self.window.firstResponder;
  if (responder == self) return YES;
  if (![responder isKindOfClass:[NSView class]]) return NO;
  return [static_cast<NSView*>(responder) isDescendantOf:self];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  wvp::mac::guarded([self] {
    if (!self.window) return;
    limitFloatingWindow(self.window);
    [self.owner viewAppeared];
  });
}

- (void)viewDidUnhide {
  [super viewDidUnhide];
  wvp::mac::guarded([self] { [self.owner viewAppeared]; });
}

@end
