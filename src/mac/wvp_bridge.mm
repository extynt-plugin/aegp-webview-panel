// The page-to-plugin half of the message bridge: WKScriptMessageHandler.
//
// The handler runs on the main thread but OUTSIDE any After Effects hook, where AEGP calls are unsupported.
// So it only translates the message into a Job and queues it (wvp::submitJob); the idle hook runs the job and
// the job's reply evaluates the answer in the page. See src/ae/wvp_entry.cpp.
#import "wvp_mac.h"

#include <utility>

#include "wvp_host.h"
#include "wvp_log.h"
#include "wvp_scripts.h"
#include "wvp_trust.h"

namespace {

using namespace wvp::mac;

// The reply holds the panel weakly: a panel closed while its request ran simply gets no answer. It also
// remembers which page load asked, and is dropped if the page has been reloaded or navigated since (request
// ids restart at 1 in every page, so a late answer could otherwise resolve the wrong promise).
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

}  // namespace

@implementation WvpPanel (Bridge)

// message.body is {id: number, op: string, args: string} (see the shim in src/core/wvp_scripts.cpp).
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
