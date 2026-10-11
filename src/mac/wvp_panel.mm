// WvpPanel: creates the web view, loads the bundled page (or, in debug builds, a development server) and
// keeps the page's theme up to date. The delegates are in wvp_panel_delegates.mm; the message bridge in
// wvp_bridge.mm.
#import "wvp_mac.h"

#include <utility>

#include "wvp_host.h"
#include "wvp_log.h"
#include "wvp_platform.h"
#include "wvp_scripts.h"
#include "wvp_trust.h"

#ifndef WVP_DEBUG
#define WVP_DEBUG 0
#endif

namespace {

using namespace wvp::mac;

NSString* const kHandlerName = @"wvp";

void addScript(WKUserContentController* controller, const std::string& source, WKUserScriptInjectionTime when,
               BOOL mainFrameOnly) {
  [controller addUserScript:[[WKUserScript alloc] initWithSource:nsString(source)
                                                   injectionTime:when
                                                forMainFrameOnly:mainFrameOnly]];
}

// Non-persistent data store: cookies, local storage and caches die with the panel. Change it if your page
// needs to remember things (WKWebsiteDataStore defaultDataStore).
WKWebViewConfiguration* makeConfiguration(WvpPanel* panel) {
  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  configuration.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
  // allowsInlinePredictions is macOS 14+; set through KVC so an older system still runs.
  if ([configuration respondsToSelector:@selector(setAllowsInlinePredictions:)])
    [configuration setValue:@NO forKey:@"allowsInlinePredictions"];
  if (WVP_DEBUG) [configuration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  [configuration.userContentController addScriptMessageHandler:panel name:kHandlerName];
  return configuration;
}

// Debug builds only: the first line of dev-url.txt in ~/Library/Application Support/aegp-webview-panel, e.g.
// http://localhost:5173/ (loopback addresses only). A release build never reads it, because the bridge can
// run ExtendScript.
NSURL* developmentUrl() {
  if (!WVP_DEBUG) return nil;
  NSString* path = [NSHomeDirectory()
      stringByAppendingPathComponent:@"Library/Application Support/aegp-webview-panel/dev-url.txt"];
  NSString* text = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  NSString* line = [text stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  NSURL* url = line.length > 0 ? [NSURL URLWithString:line] : nil;
  if (!url) return nil;
  if (wvp::isLoopbackOrigin(wvp::originOf(stdString(url.absoluteString)))) return url;
  wvp::logLine("dev-url.txt ignored: only localhost, 127.0.0.1 and [::1] are allowed");
  return nil;
}

// Contents/Resources/web of the plugin bundle (the bundle this code was loaded from).
NSURL* bundledWebFolder() {
  NSBundle* bundle = [NSBundle bundleForClass:[WvpWebView class]];
  return [bundle.resourceURL URLByAppendingPathComponent:@"web"];
}

// The web folder as a URL prefix: "file:///.../web/" (a trailing slash keeps "web-other" from matching).
NSString* bundledWebPrefix() {
  std::string text = stdString(bundledWebFolder().absoluteString);
  if (text.empty() || text.back() != '/') text += '/';
  return nsString(text);
}

}  // namespace

@implementation WvpPanel

- (void)attachTo:(NSView*)container {
  WKWebViewConfiguration* configuration = makeConfiguration(self);
  WvpWebView* web = [[WvpWebView alloc] initWithFrame:container.bounds configuration:configuration];
  [liveViews() addObject:web];
  web.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;  // follows the panel when it is resized
  if (@available(macOS 13.3, *)) web.inspectable = WVP_DEBUG ? YES : NO;
  web.owner = self;
  web.navigationDelegate = self;
  web.UIDelegate = self;
  self.web = web;
  [container addSubview:web];
}

- (void)installUserScripts {
  WKUserContentController* controller = self.web.configuration.userContentController;
  // The theme is the last one the idle hook reported (hostTheme() must not be called from here, outside a
  // hook).
  const std::string themeJson = self.themeJson.length > 0 ? stdString(self.themeJson) : "null";
  self.generation += 1;
  [controller removeAllUserScripts];
  addScript(controller, wvp::bridgeShimScript(themeJson), WKUserScriptInjectionTimeAtDocumentStart, YES);
  addScript(controller, wvp::fieldHardeningScript(), WKUserScriptInjectionTimeAtDocumentEnd, NO);
}

- (void)loadPage {
  guarded([self] {
    WvpWebView* web = self.web;
    if (!web) return;
    self.webPrefix = bundledWebPrefix();
    [self installUserScripts];
    [self pushTheme];
    NSURL* development = developmentUrl();
    if (development) {
      self.devOrigin = nsString(wvp::originOf(stdString(development.absoluteString)));
      wvp::logLine("loading the development server " + stdString(development.absoluteString));
      [web loadRequest:[NSURLRequest requestWithURL:development]];
      return;
    }
    self.devOrigin = @"";
    NSURL* folder = bundledWebFolder();
    NSURL* index = [folder URLByAppendingPathComponent:@"index.html"];
    wvp::logLine("loading " + stdString(index.absoluteString));
    [web loadFileURL:index allowingReadAccessToURL:[index URLByDeletingLastPathComponent]];
  });
}

// WKScriptMessageHandler: a required method, so it lives here with the class; the work is in wvp_bridge.mm.
- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message {
  guarded([self, message] { [self handleBridgeMessage:message]; });
}

- (void)viewAppeared {
  guarded([self] { [self pushTheme]; });
}

// AE announces no brightness change to a plugin, so the colour is re-read whenever a window becomes key or
// the app becomes active (wvp_install.mm), which also covers closing AE's Preferences. The read itself is a
// job for the idle hook ("host.theme"): this method runs outside any AE hook, where PF suites are
// unsupported.
- (void)pushTheme {
  guarded([self] {
    if (!self.web) return;
    __weak WvpPanel* weakSelf = self;
    wvp::Job job;
    job.op = "host.theme";
    job.reply = [weakSelf](const wvp::OpResult& result) {
      WvpPanel* panel = weakSelf;
      if (panel && result.ok) guarded([panel, &result] { [panel themeArrived:nsString(result.json)]; });
    };
    wvp::submitJob(std::move(job));
  });
}

- (void)themeArrived:(NSString*)json {
  if ([json isEqualToString:@"null"] || [json isEqualToString:self.themeJson] || !self.web) return;
  self.themeJson = json;
  [self.web evaluateJavaScript:nsString(wvp::themeScript(stdString(json))) completionHandler:nil];
}

// The answer to a theme request can arrive before the new page has defined window.wvp; send it again once the
// page has loaded.
- (void)pageFinishedLoading {
  if (self.themeJson.length == 0 || !self.web) return;
  [self.web evaluateJavaScript:nsString(wvp::themeScript(stdString(self.themeJson))) completionHandler:nil];
}

@end
