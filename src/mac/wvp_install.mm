// Process-wide macOS setup: WebKit text-correction defaults (in memory only), the key monitor, and the
// observers that re-read AE's theme when a window becomes key or the app becomes active.
#import "wvp_mac.h"

#include "wvp_log.h"
#include "wvp_platform.h"

namespace {

using namespace wvp::mac;

NSMutableArray* g_observers = nil;

void registerQuietWebDefaults() {
  [[NSUserDefaults standardUserDefaults] registerDefaults:@{
    @"WebAutomaticSpellingCorrectionEnabled" : @NO,
    @"WebAutomaticTextReplacementEnabled" : @NO,
    @"WebContinuousSpellCheckingEnabled" : @NO,
    @"WebAutomaticQuoteSubstitutionEnabled" : @NO,
    @"WebAutomaticDashSubstitutionEnabled" : @NO,
    @"WebGrammarCheckingEnabled" : @NO,
    @"WebAutomaticLinkDetectionEnabled" : @NO,
    @"NSAutoFillHeuristicControllerEnabled" : @NO
  }];
}

void pushThemeToAllPanels() {
  for (WvpWebView* web in liveViews().allObjects) [web.owner pushTheme];
}

void observeTheme() {
  g_observers = [NSMutableArray array];
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  for (NSNotificationName name in
       @[ NSWindowDidBecomeKeyNotification, NSApplicationDidBecomeActiveNotification ]) {
    id token = [center addObserverForName:name
                                   object:nil
                                    queue:[NSOperationQueue mainQueue]
                               usingBlock:^(NSNotification*) { guarded([] { pushThemeToAllPanels(); }); }];
    [g_observers addObject:token];
  }
}

}

namespace wvp::platform {

void install() {
  registerQuietWebDefaults();
  installMonitor();
  observeTheme();
}

void uninstall() {
  removeMonitor();
  for (id token in g_observers) [[NSNotificationCenter defaultCenter] removeObserver:token];
  g_observers = nil;
}

void* createPanel(void* nativeView) {
  NSView* container = (__bridge NSView*)nativeView;
  if (!container) return nullptr;
  WvpPanel* panel = [[WvpPanel alloc] init];
  [panel attachTo:container];
  [panel loadPage];
  return const_cast<void*>(CFBridgingRetain(panel));
}

void reloadPanel(void* refcon) {
  if (refcon == nullptr) return;
  WvpPanel* panel = (__bridge WvpPanel*)refcon;
  [panel loadPage];
}

bool keyLoggingEnabled() { return g_logKeys.load(); }

void setKeyLogging(bool enabled) {
  logLine(enabled ? "key logging on" : "key logging off");
  g_logKeys.store(enabled);
}

}
