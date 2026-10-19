// The web view subclass (takes the first click, knows whether it holds focus, gives floating windows a
// minimum size) and the helpers shared by the macOS files.
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

}

namespace {

void limitFloatingWindow(NSWindow* window) {
  if (window == NSApp.mainWindow) return;
  const NSSize now = window.contentMinSize;
  window.contentMinSize = NSSize{std::max(now.width, static_cast<CGFloat>(wvp::platform::kMinimumWidth)),
                                 std::max(now.height, static_cast<CGFloat>(wvp::platform::kMinimumHeight))};
}

}

@implementation WvpWebView

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
