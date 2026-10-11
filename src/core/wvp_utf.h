// UTF-16 to UTF-8. After Effects hands names (items, layers, paths) over as null-terminated UTF-16.
#pragma once

#include <string>

namespace wvp {

// Converts the null-terminated UTF-16 text at `text` (nullptr gives ""). A lone surrogate becomes U+FFFD.
std::string utf16ToUtf8(const char16_t* text);

}  // namespace wvp
