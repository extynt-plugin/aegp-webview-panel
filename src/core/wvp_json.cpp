#include "wvp_json.h"

#include <cstddef>

namespace wvp {

namespace {

constexpr char kHex[] = "0123456789abcdef";

void appendUnicodeEscape(std::string& out, unsigned value) {
  out += "\\u";
  for (int shift = 12; shift >= 0; shift -= 4) out += kHex[(value >> static_cast<unsigned>(shift)) & 0xFU];
}

// True for the three bytes E2 80 A8 / E2 80 A9 (U+2028, U+2029) starting at `at`.
bool isLineSeparator(const std::string& text, std::size_t at) {
  if (at + 2 >= text.size()) return false;
  const auto byte = [&text](std::size_t index) { return static_cast<unsigned char>(text[index]); };
  return byte(at) == 0xE2U && byte(at + 1) == 0x80U && (byte(at + 2) == 0xA8U || byte(at + 2) == 0xA9U);
}

// Appends the escape for a single-byte character; returns false when the byte is copied as it is.
bool appendEscaped(std::string& out, char c) {
  switch (c) {
    case '"':
      out += "\\\"";
      return true;
    case '\\':
      out += "\\\\";
      return true;
    case '\n':
      out += "\\n";
      return true;
    case '\r':
      out += "\\r";
      return true;
    case '\t':
      out += "\\t";
      return true;
    default:
      break;
  }
  if (static_cast<unsigned char>(c) >= 0x20U) return false;
  appendUnicodeEscape(out, static_cast<unsigned char>(c));
  return true;
}

}  // namespace

std::string jsonQuote(const std::string& utf8) {
  std::string out = "\"";
  for (std::size_t i = 0; i < utf8.size(); ++i) {
    if (isLineSeparator(utf8, i)) {
      appendUnicodeEscape(out, static_cast<unsigned char>(utf8[i + 2]) == 0xA8U ? 0x2028U : 0x2029U);
      i += 2;
    } else if (!appendEscaped(out, utf8[i])) {
      out += utf8[i];
    }
  }
  out += '"';
  return out;
}

}  // namespace wvp
