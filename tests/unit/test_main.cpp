// Unit tests for the platform-neutral code. A few lines of harness instead of a framework: the tests run
// anywhere with a C++17 compiler (CI runs them on Linux).
#include <cstdio>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "wvp_json.h"
#include "wvp_keys.h"
#include "wvp_queue.h"
#include "wvp_scripts.h"
#include "wvp_theme.h"
#include "wvp_trust.h"
#include "wvp_utf.h"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (ok) return;
  ++g_failures;
  std::fprintf(stderr, "FAIL line %d: %s\n", line, what);
}

#define CHECK(expr) check((expr), #expr, __LINE__)

void testJsonQuote() {
  CHECK(wvp::jsonQuote("plain") == "\"plain\"");
  CHECK(wvp::jsonQuote("a\"b\\c") == "\"a\\\"b\\\\c\"");
  CHECK(wvp::jsonQuote("l1\nl2\t") == "\"l1\\nl2\\t\"");
  CHECK(wvp::jsonQuote(std::string("\x01", 1)) == "\"\\u0001\"");
  CHECK(wvp::jsonQuote("\xE2\x80\xA8") == "\"\\u2028\"");  // U+2028 would end a JavaScript string literal
  CHECK(wvp::jsonQuote("\xE2\x80\xA9") == "\"\\u2029\"");
  CHECK(wvp::jsonQuote("caf\xC3\xA9") == "\"caf\xC3\xA9\"");  // other UTF-8 passes through
  CHECK(wvp::jsonQuote("\xE2\x80") == "\"\xE2\x80\"");        // a truncated sequence is copied, not misread
}

void testUtf16() {
  CHECK(wvp::utf16ToUtf8(nullptr).empty());
  CHECK(wvp::utf16ToUtf8(u"Comp 1") == "Comp 1");
  CHECK(wvp::utf16ToUtf8(u"caf\u00E9") == "caf\xC3\xA9");
  CHECK(wvp::utf16ToUtf8(u"\u5EAD") == "\xE5\xBA\xAD");
  CHECK(wvp::utf16ToUtf8(u"\U0001F600") == "\xF0\x9F\x98\x80");  // a surrogate pair
  CHECK(wvp::utf16ToUtf8(u"a\xD800"
                         "b") ==
        "a\xEF\xBF\xBD"
        "b");                                            // lone high surrogate
  CHECK(wvp::utf16ToUtf8(u"\xDC00") == "\xEF\xBF\xBD");  // lone low surrogate
}

void testTheme() {
  const wvp::Theme dark = wvp::themeFromChannels16(0x2323, 0x2323, 0x2323);
  CHECK(dark.red == 0x23 && dark.dark);
  const wvp::Theme light = wvp::themeFromChannels16(65535, 65535, 65535);
  CHECK(light.red == 255 && !light.dark && light.luminance > 0.99);
  CHECK(wvp::themeFromChannels16(-5, 70000, 0).green == 255);  // clamped
  CHECK(wvp::themeCss(light) == "rgb(255, 255, 255)");
  CHECK(wvp::themeJson(light).find("\"dark\":false") != std::string::npos);
  CHECK(wvp::themeJson(light).find("\"luminance\":1.000") != std::string::npos);
}

void testScripts() {
  CHECK(wvp::replyScript(7, true, "\"hi\"") == "window.wvp._resolve(7,true,\"\\\"hi\\\"\");");
  CHECK(wvp::bridgeShimScript("null").find("wvp.theme = null;") != std::string::npos);
  CHECK(wvp::themeScript("{}") == "window.wvp&&window.wvp._setTheme({});");
  CHECK(wvp::fieldHardeningScript().find("writingsuggestions") != std::string::npos);
}

void testTrust() {
  CHECK(wvp::originOf("http://localhost:5173/a/b?c=d") == "http://localhost:5173");
  CHECK(wvp::originOf("https://example.com") == "https://example.com");
  CHECK(wvp::originOf("file:///tmp/x").empty());
  const std::string web = "file:///App/Contents/Resources/web/";
  CHECK(wvp::isTrustedPageUrl(web + "index.html", "", web));
  CHECK(!wvp::isTrustedPageUrl("file:///Users/x/evil.html", "", web));
  CHECK(!wvp::isTrustedPageUrl("file:///App/Contents/Resources/webevil/index.html", "", web));
  CHECK(!wvp::isTrustedPageUrl(web + "../../../evil.html", "", web));
  CHECK(!wvp::isTrustedPageUrl(web + "%2e%2e/evil.html", "", web));
  CHECK(!wvp::isTrustedPageUrl(web + "index.html", "", ""));  // no prefix known: no file is trusted
  CHECK(!wvp::isTrustedPageUrl("http://localhost:5173/", "", web));
  CHECK(wvp::isTrustedPageUrl("http://localhost:5173/x", "http://localhost:5173", web));
  CHECK(!wvp::isTrustedPageUrl("http://localhost:5173.evil.com/", "http://localhost:5173", web));
  CHECK(wvp::isLoopbackOrigin("http://localhost:5173"));
  CHECK(wvp::isLoopbackOrigin("http://127.0.0.1"));
  CHECK(wvp::isLoopbackOrigin("http://[::1]:8080"));
  CHECK(!wvp::isLoopbackOrigin("http://example.com"));
  CHECK(!wvp::isLoopbackOrigin("http://localhost.evil.com"));
  CHECK(!wvp::isLoopbackOrigin("http://192.168.1.5:5173"));
  CHECK(!wvp::isLoopbackOrigin("http://localhost:80x"));
  CHECK(!wvp::isLoopbackOrigin("http://[::2]"));
  CHECK(!wvp::isLoopbackOrigin("http://localhost/path"));
  using wvp::Navigation;
  CHECK(wvp::decideNavigation("about:blank", false, "", web) == Navigation::Allow);
  CHECK(wvp::decideNavigation(web + "index.html", false, "", web) == Navigation::Allow);
  CHECK(wvp::decideNavigation("file:///Users/x/evil.html", false, "", web) == Navigation::Cancel);
  CHECK(wvp::decideNavigation("https://example.com/", true, "", web) == Navigation::OpenExternal);
  CHECK(wvp::decideNavigation("https://example.com/", false, "", web) ==
        Navigation::Cancel);  // a scripted redirect
  CHECK(wvp::decideNavigation("javascript:alert(1)", true, "", web) == Navigation::Cancel);
}

void testQueue() {
  wvp::JobQueue queue;
  CHECK(!queue.pop().has_value());
  std::vector<std::thread> producers;
  producers.reserve(4);
  for (int t = 0; t < 4; ++t) {
    producers.emplace_back([&queue] {
      for (int i = 0; i < 50; ++i) queue.push(wvp::Job{"op", "", nullptr});
    });
  }
  for (std::thread& producer : producers) producer.join();
  CHECK(queue.size() == 200);
  int popped = 0;
  while (queue.pop()) ++popped;
  CHECK(popped == 200);
  queue.push(wvp::Job{"first", "", nullptr});
  queue.push(wvp::Job{"second", "", nullptr});
  const std::optional<wvp::Job> first = queue.pop();
  CHECK(first.has_value() && first->op == "first");  // first in, first out
}

wvp::mac::KeyInput key(unsigned short code, char base, bool command, bool shift = false) {
  wvp::mac::KeyInput input;
  input.keyCode = code;
  input.base = base;
  input.command = command;
  input.shift = shift;
  return input;
}

void testKeyRouting() {
  using wvp::mac::Action;
  using wvp::mac::Route;
  CHECK(wvp::mac::classifyKey(key(8, 'c', true)).action == Action::Copy);
  CHECK(wvp::mac::classifyKey(key(9, 'v', true)).action == Action::Paste);
  CHECK(wvp::mac::classifyKey(key(0, 'a', true)).action == Action::SelectAll);
  CHECK(wvp::mac::classifyKey(key(6, 'z', true)).action == Action::Undo);
  CHECK(wvp::mac::classifyKey(key(6, 'z', true, true)).action == Action::Redo);
  CHECK(wvp::mac::classifyKey(key(1, 's', true)).route == Route::Pass);  // Cmd+S is After Effects'
  CHECK(wvp::mac::classifyKey(key(0, 'a', true, true)).route ==
        Route::Pass);  // Shift+Cmd+A is After Effects'
  CHECK(wvp::mac::classifyKey(key(123, 0, true)).route ==
        Route::Deliver);  // Cmd+Left: line start in a text field
  CHECK(wvp::mac::classifyKey(key(0, 'a', false)).route == Route::Deliver);  // a plain letter
  CHECK(wvp::mac::classifyKey(key(124, 0, false)).route == Route::Deliver);  // an arrow
  CHECK(wvp::mac::classifyKey(key(115, 0, false)).action == Action::LineStart);
  CHECK(wvp::mac::classifyKey(key(119, 0, false, true)).action == Action::LineEndSelect);
  wvp::mac::KeyInput control = key(8, 'c', false);
  control.control = true;
  CHECK(wvp::mac::classifyKey(control).route == Route::Pass);
  wvp::mac::KeyInput commandOption = key(8, 'c', true);
  commandOption.option = true;
  CHECK(wvp::mac::classifyKey(commandOption).route == Route::Pass);
  CHECK(wvp::mac::actionSelector(Action::Copy) == "copy:");
  CHECK(wvp::mac::actionJsFallback(Action::Paste).empty());
}

void testResend() {
  wvp::mac::KeyStamp routed;
  routed.timestamp = 1234.5;
  routed.keyCode = 115;
  routed.type = 10;
  routed.windowNumber = 4;
  routed.valid = true;
  wvp::mac::KeyStamp same = routed;
  CHECK(wvp::mac::isResendOf(routed, same));
  same.timestamp = 1234.6;  // a later press of the same key
  CHECK(!wvp::mac::isResendOf(routed, same));
  same = routed;
  same.type = 11;  // the key up
  CHECK(!wvp::mac::isResendOf(routed, same));
  CHECK(!wvp::mac::isResendOf(wvp::mac::KeyStamp{}, wvp::mac::KeyStamp{}));
}

}  // namespace

int main() {
  testJsonQuote();
  testUtf16();
  testTheme();
  testScripts();
  testTrust();
  testQueue();
  testKeyRouting();
  testResend();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
