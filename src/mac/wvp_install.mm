// The macOS half of wvp_platform.h that is not about one panel: process-wide setup and the entry points the
// AEGP side calls.
#import "wvp_mac.h"

#include "wvp_log.h"
#include "wvp_platform.h"

namespace {

using namespace wvp::mac;

NSMutableArray* g_observers = nil;

// WebKit's autocorrect, text replacement and spellcheck popups run a nested event loop that stalls After
// Effects' idle hook. WebKit reads these keys from the standard defaults; registering them fills only the
// in-memory registration domain, so nothing is written to After Effects' preferences.
void registerQuietWebDefaults() {
  [[NSUserDefaults standardUserDefaults] registerDefaults:@{
    @"WebAutomaticSpellingCorrectionEnabled" : @NO,
    @"WebAutomaticTextReplacementEnabled" : @NO,
    @"WebContinuousSpellCheckingEnabled" : @NO,
    @"WebAutomaticQuoteSubstitutionEnabled" : @NO,
    @"WebAutomaticDashSubstitutionEnabled" : @NO,
    @"WebGrammarCheckingEnabled" : @NO,
    @"WebAutomaticLinkDetectionEnabled" : @NO,
    @"NSAutoFillHeuristicControllerEnabled" : @NO  // AppKit adds "AutoFill >" to the context menu otherwise
  }];
}

void pushThemeToAllPanels() {
  for (WvpWebView* web in liveViews().allObjects) [web.owner pushTheme];
}

// After Effects announces no brightness change to a plugin. A theme change needs the Preferences window, so
// the colour is re-read when a window becomes key or the application becomes active (which covers closing
// Preferences).
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

}  // namespace

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
  return const_cast<void*>(CFBridgingRetain(panel));  // never released: After Effects has no destroy callback
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

}  // namespace wvp::platform
