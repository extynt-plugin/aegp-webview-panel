// Objective-C++ only. The macOS panel: a WKWebView subclass, and the per-panel object that is its navigation
// delegate, UI delegate and the receiver of the page's script messages. Compiled with ARC.
#pragma once

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <atomic>
#include <string>

#include "wvp_log.h"

@class WvpPanel;

@interface WvpWebView : WKWebView
@property(weak) WvpPanel* owner;  // told when the view comes back into a window or is shown again
// YES when the window's first responder is this view or one of its subviews (WebKit's content view).
- (BOOL)firstResponderIsInside;
@end

// One per panel. After Effects gives no destroy callback, so this object (the panel refcon) is retained for
// the life of the process; the web view itself is only weakly held here.
@interface WvpPanel : NSObject <WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>
@property(weak) WvpWebView* web;
@property(copy) NSString* devOrigin;  // the origin of the development server page, "" when none is loaded
@property(copy)
    NSString* webPrefix;  // file URL of the bundle's web folder, ending in "/": the only trusted files
@property(copy)
    NSString* themeJson;  // the host theme the page was last told about, "" before the first answer
@property(assign) NSUInteger generation;  // bumped on every page load; replies from an older page are dropped
- (void)attachTo:(NSView*)container;
- (void)loadPage;
- (void)installUserScripts;  // replaces the user scripts so they carry the current host theme
- (void)viewAppeared;        // the view moved into a window or was unhidden
- (void)pushTheme;           // asks the idle hook for the AE UI colour and tells the page if it changed
- (void)themeArrived:(NSString*)json;  // main thread: the idle hook answered a theme request
- (void)pageFinishedLoading;           // sends the page the theme it may have missed while loading
@end

@interface WvpPanel (Delegates)  // wvp_panel_delegates.mm: navigation policy, dialogs, crash recovery
@end

@interface WvpPanel (Bridge)  // wvp_bridge.mm: the page-to-plugin message handler
- (void)handleBridgeMessage:(WKScriptMessage*)message;
@end

namespace wvp::mac {

extern std::atomic<bool> g_logKeys;

// Every live web view (weak references, so a destroyed panel drops out by itself). Main thread only.
NSHashTable* liveViews();

std::string stdString(NSString* text);
NSString* nsString(const std::string& text);
std::string className(id object);

// Runs `work` and swallows anything it throws: no exception may be thrown into Cocoa or WebKit.
template <typename Work>
void guarded(const Work& work) noexcept {
  try {
    work();
  } catch (...) {
    logDroppedException("cocoa callback");
  }
}

// The NSEvent local monitor (wvp_monitor.mm).
void installMonitor();
void removeMonitor();

}  // namespace wvp::mac
