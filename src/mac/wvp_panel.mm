// Creates the web view, loads the bundled page (or a loopback dev server named in dev-url.txt, debug builds
// only) and keeps the page's theme current. The data store is non-persistent, so storage dies with the panel.
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

WKWebViewConfiguration* makeConfiguration(WvpPanel* panel) {
  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  configuration.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
  if ([configuration respondsToSelector:@selector(setAllowsInlinePredictions:)])
    [configuration setValue:@NO forKey:@"allowsInlinePredictions"];
  if (WVP_DEBUG) [configuration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  [configuration.userContentController addScriptMessageHandler:panel name:kHandlerName];
  return configuration;
}

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

NSURL* bundledWebFolder() {
  NSBundle* bundle = [NSBundle bundleForClass:[WvpWebView class]];
  return [bundle.resourceURL URLByAppendingPathComponent:@"web"];
}

NSString* bundledWebPrefix() {
  std::string text = stdString(bundledWebFolder().absoluteString);
  if (text.empty() || text.back() != '/') text += '/';
  return nsString(text);
}

}

@implementation WvpPanel

- (void)attachTo:(NSView*)container {
  WKWebViewConfiguration* configuration = makeConfiguration(self);
  WvpWebView* web = [[WvpWebView alloc] initWithFrame:container.bounds configuration:configuration];
  [liveViews() addObject:web];
  web.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  if (@available(macOS 13.3, *)) web.inspectable = WVP_DEBUG ? YES : NO;
  web.owner = self;
  web.navigationDelegate = self;
  web.UIDelegate = self;
  self.web = web;
  [container addSubview:web];
}

- (void)installUserScripts {
  WKUserContentController* controller = self.web.configuration.userContentController;
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

- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message {
  guarded([self, message] { [self handleBridgeMessage:message]; });
}

- (void)viewAppeared {
  guarded([self] { [self pushTheme]; });
}

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

- (void)pageFinishedLoading {
  if (self.themeJson.length == 0 || !self.web) return;
  [self.web evaluateJavaScript:nsString(wvp::themeScript(stdString(self.themeJson))) completionHandler:nil];
}

@end
