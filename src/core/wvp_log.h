// One logging function, implemented per platform (src/mac/wvp_log_mac.mm writes to NSLog, which shows up in
// Console.app under the After Effects process). Safe to call from any thread.
#pragma once

#include <string>

namespace wvp {

void logLine(const std::string& line);

// Called where a C++ exception would otherwise cross into After Effects or Cocoa. Logs which boundary dropped
// it.
void logDroppedException(const char* where);

}  // namespace wvp
