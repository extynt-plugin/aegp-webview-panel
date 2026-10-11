// The navigation and UI delegates of WvpPanel.
#import "wvp_mac.h"

#include "wvp_log.h"
#include "wvp_trust.h"

namespace {

using namespace wvp::mac;

WKNavigationActionPolicy policyFor(WvpPanel* panel, WKNavigationAction* action) {
  NSURL* url = action.request.URL;
  const bool linkActivated = action.navigationType == WKNavigationTypeLinkActivated;
  const std::string text = url ? stdString(url.absoluteString) : std::string();
  switch (
      wvp::decideNavigation(text, linkActivated, stdString(panel.devOrigin), stdString(panel.webPrefix))) {
    case wvp::Navigation::Allow:
      if (action.targetFrame.mainFrame && text != "about:blank") [panel installUserScripts];
      return WKNavigationActionPolicyAllow;
    case wvp::Navigation::OpenExternal:
      [[NSWorkspace sharedWorkspace] openURL:url
                               configuration:[NSWorkspaceOpenConfiguration configuration]
                           completionHandler:nil];
      return WKNavigationActionPolicyCancel;
    case wvp::Navigation::Cancel:
      break;
  }
  wvp::logLine("navigation refused: " + text);
  return WKNavigationActionPolicyCancel;
}

}  // namespace

@implementation WvpPanel (Delegates)

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
  WKNavigationActionPolicy policy = WKNavigationActionPolicyCancel;
  guarded([&] { policy = policyFor(self, navigationAction); });
  decisionHandler(policy);
}

// WebKit's web content process died (out of memory, a crash): load the page again.
- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
  guarded([self] {
    wvp::logLine("the web content process terminated, reloading");
    [self loadPage];
  });
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation {
  guarded([self] { [self pageFinishedLoading]; });
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
  guarded([error] { wvp::logLine("page failed to load: " + stdString(error.localizedDescription)); });
}

// JavaScript dialogs and file pickers are answered at once. Showing one would run a nested event loop inside
// After Effects' main thread, which stalls its idle hook for as long as the dialog is open (README,
// "Gotchas").
- (void)webView:(WKWebView*)webView
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler {
  completionHandler();
}

- (void)webView:(WKWebView*)webView
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                        initiatedByFrame:(WKFrameInfo*)frame
                       completionHandler:(void (^)(BOOL))completionHandler {
  completionHandler(NO);
}

- (void)webView:(WKWebView*)webView
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler {
  completionHandler(nil);
}

- (void)webView:(WKWebView*)webView
    runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))completionHandler {
  completionHandler(nil);
}

@end
