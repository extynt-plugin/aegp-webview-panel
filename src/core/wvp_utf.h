// Converts the null-terminated UTF-16 that AE uses for names to UTF-8. A lone surrogate becomes U+FFFD.
#pragma once

#include <string>

namespace wvp {

std::string utf16ToUtf8(const char16_t* text);

}
