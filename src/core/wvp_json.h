// Tiny JSON helpers. The bridge passes JSON text between the page and the plugin, and the only thing the
// native side has to produce is a correctly quoted string.
#pragma once

#include <string>

namespace wvp {

// `utf8` as a double-quoted JSON string literal that is also a valid JavaScript string literal: quote,
// backslash and control characters are escaped, and so are U+2028 and U+2029 (line terminators in older
// JavaScript engines).
std::string jsonQuote(const std::string& utf8);

}  // namespace wvp
