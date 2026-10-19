// The page-to-plugin message handler. It runs outside any AE hook, so it only queues a Job. The reply holds
// the panel weakly and is dropped if the page has reloaded since, because request ids restart in every page.
#import "wvp_mac.h"

#include <utility>

#include "wvp_host.h"
#include "wvp_log.h"
#include "wvp_scripts.h"
#include "wvp_trust.h"

namespace {

using namespace wvp::mac;

wvp::Reply replyTo(WvpPanel* panel, long long requestId) {
  __weak WvpPanel* weakPanel = panel;
  const NSUInteger generation = panel.generation;
  return [weakPanel, generation, requestId](const wvp::OpResult& result) {
    WvpPanel* owner = weakPanel;
    WvpWebView* target = owner.web;
    if (!target || owner.generation != generation) return;
    NSString* script = nsString(wvp::replyScript(requestId, result.ok, result.json));
    [target evaluateJavaScript:script completionHandler:nil];
  };
}

bool isTrusted(WvpPanel* panel, WKScriptMessage* message) {
  NSURL* url = message.frameInfo.request.URL;
  const std::string text = url ? stdString(url.absoluteString) : std::string();
  return message.frameInfo.mainFrame &&
         wvp::isTrustedPageUrl(text, stdString(panel.devOrigin), stdString(panel.webPrefix));
}

}

@implementation WvpPanel (Bridge)

- (void)handleBridgeMessage:(WKScriptMessage*)message {
  if (!isTrusted(self, message)) {
    wvp::logLine("bridge message from an untrusted page ignored");
    return;
  }
  if (![message.body isKindOfClass:[NSDictionary class]]) return;
  NSDictionary* body = message.body;
  id requestId = [body objectForKey:@"id"];
  id op = [body objectForKey:@"op"];
  id args = [body objectForKey:@"args"];
  if (![requestId isKindOfClass:[NSNumber class]] || ![op isKindOfClass:[NSString class]]) return;
  wvp::Job job;
  job.op = stdString(op);
  job.args = [args isKindOfClass:[NSString class]] ? stdString(args) : std::string();
  job.reply = replyTo(self, [requestId longLongValue]);
  wvp::submitJob(std::move(job));
}

@end
