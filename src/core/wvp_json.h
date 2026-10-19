// JSON string quoting. The output is also a valid JavaScript string literal.
#pragma once

#include <string>

namespace wvp {

std::string jsonQuote(const std::string& utf8);

}
