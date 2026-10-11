#include "wvp_utf.h"

namespace wvp {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

void appendUtf8(std::string& out, char32_t cp) {
  if (cp < 0x80U) {
    out += static_cast<char>(cp);
  } else if (cp < 0x800U) {
    out += static_cast<char>(0xC0U | (cp >> 6U));
    out += static_cast<char>(0x80U | (cp & 0x3FU));
  } else if (cp < 0x10000U) {
    out += static_cast<char>(0xE0U | (cp >> 12U));
    out += static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU));
    out += static_cast<char>(0x80U | (cp & 0x3FU));
  } else {
    out += static_cast<char>(0xF0U | (cp >> 18U));
    out += static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU));
    out += static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU));
    out += static_cast<char>(0x80U | (cp & 0x3FU));
  }
}

bool isHigh(char16_t unit) { return unit >= 0xD800U && unit <= 0xDBFFU; }
bool isLow(char16_t unit) { return unit >= 0xDC00U && unit <= 0xDFFFU; }

// The code point that starts at `*text`; advances `*text` past it.
char32_t nextCodePoint(const char16_t** text) {
  const char16_t first = **text;
  ++*text;
  if (isLow(first)) return kReplacement;
  if (!isHigh(first)) return first;
  const char16_t second = **text;
  if (!isLow(second)) return kReplacement;
  ++*text;
  return 0x10000U + ((static_cast<char32_t>(first) - 0xD800U) << 10U) +
         (static_cast<char32_t>(second) - 0xDC00U);
}

}  // namespace

std::string utf16ToUtf8(const char16_t* text) {
  std::string out;
  while (text != nullptr && *text != 0) appendUtf8(out, nextCodePoint(&text));
  return out;
}

}  // namespace wvp
