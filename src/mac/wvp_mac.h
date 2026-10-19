// Objective-C++ only, compiled with ARC. The macOS panel: a WKWebView subclass and the per-panel object that
// is its delegate and message handler. AE never destroys a panel, so that object lives as long as AE.
#pragma once

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <atomic>
#include <string>

#include "wvp_log.h"

@class WvpPanel;

@interface WvpWebView : WKWebView
@property(weak) WvpPanel* owner;
- (BOOL)firstResponderIsInside;
@end

@interface WvpPanel : NSObject <WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>
@property(weak) WvpWebView* web;
@property(copy) NSString* devOrigin;
@property(copy) NSString* webPrefix;
@property(copy) NSString* themeJson;
@property(assign) NSUInteger generation;
- (void)attachTo:(NSView*)container;
- (void)loadPage;
- (void)installUserScripts;
- (void)viewAppeared;
- (void)pushTheme;
- (void)themeArrived:(NSString*)json;
- (void)pageFinishedLoading;
@end

@interface WvpPanel (Delegates)
@end

@interface WvpPanel (Bridge)
- (void)handleBridgeMessage:(WKScriptMessage*)message;
@end

namespace wvp::mac {

extern std::atomic<bool> g_logKeys;

NSHashTable* liveViews();

std::string stdString(NSString* text);
NSString* nsString(const std::string& text);
std::string className(id object);

template <typename Work>
void guarded(const Work& work) noexcept {
  try {
    work();
  } catch (...) {
    logDroppedException("cocoa callback");
  }
}

void installMonitor();
void removeMonitor();

}
