// The macOS logger: NSLog, visible in Console.app under the After Effects process (filter on "[wvp]").
#import <Foundation/Foundation.h>

#include <exception>

#include "wvp_log.h"

namespace wvp {

void logLine(const std::string& line) { NSLog(@"[wvp] %s", line.c_str()); }

void logDroppedException(const char* where) {
  try {
    throw;
  } catch (const std::exception& error) {
    logLine(std::string("exception dropped at ") + where + ": " + error.what());
  } catch (...) {
    logLine(std::string("unknown exception dropped at ") + where);
  }
}

}
