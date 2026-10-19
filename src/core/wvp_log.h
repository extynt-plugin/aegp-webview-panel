// Per-platform logging, safe from any thread. logDroppedException notes an exception stopped at a boundary.
#pragma once

#include <string>

namespace wvp {

void logLine(const std::string& line);

void logDroppedException(const char* where);

}
