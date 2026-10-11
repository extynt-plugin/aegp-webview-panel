#include "wvp_trust.h"

#include <cstddef>

namespace wvp {

namespace {

bool startsWith(const std::string& text, const std::string& prefix) {
  return text.compare(0, prefix.size(), prefix) == 0;
}

// ":" followed by digits, or nothing.
bool portIsValid(const std::string& port) {
  if (port.empty()) return true;
  if (port.size() < 2 || port.front() != ':') return false;
  return port.find_first_not_of("0123456789", 1) == std::string::npos;
}

// A url under the web folder that climbs out of it ("..") or hides a separator or dot in percent-encoding.
bool escapesFolder(const std::string& url) {
  std::string lower;
  for (const char c : url) lower += static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
  for (const char* bad : {"..", "%2e", "%2f", "%5c", "\\"}) {
    if (lower.find(bad) != std::string::npos) return true;
  }
  return false;
}

}  // namespace

std::string originOf(const std::string& url) {
  const std::string http = "http://";
  const std::string https = "https://";
  std::size_t start = 0;
  if (startsWith(url, http)) start = http.size();
  else if (startsWith(url, https)) start = https.size();
  else return "";
  const std::size_t end = url.find_first_of("/?#", start);
  return end == std::string::npos ? url : url.substr(0, end);
}

bool isLoopbackOrigin(const std::string& origin) {
  if (originOf(origin) != origin) return false;
  const std::size_t hostStart = origin.find("://") + 3;
  std::string host = origin.substr(hostStart);
  if (!host.empty() && host.front() == '[') {
    const std::size_t close = host.find(']');
    if (close == std::string::npos) return false;
    return host.substr(0, close + 1) == "[::1]" && portIsValid(host.substr(close + 1));
  }
  const std::size_t colon = host.find(':');
  const std::string name = host.substr(0, colon);
  const std::string port = colon == std::string::npos ? "" : host.substr(colon);
  return (name == "localhost" || name == "127.0.0.1") && portIsValid(port);
}

bool isTrustedPageUrl(const std::string& url, const std::string& devOrigin, const std::string& webPrefix) {
  if (!webPrefix.empty() && startsWith(url, webPrefix) && !escapesFolder(url)) return true;
  return !devOrigin.empty() && originOf(url) == devOrigin;
}

Navigation decideNavigation(const std::string& url, bool linkActivated, const std::string& devOrigin,
                            const std::string& webPrefix) {
  if (url == "about:blank" || isTrustedPageUrl(url, devOrigin, webPrefix)) return Navigation::Allow;
  if (linkActivated && !originOf(url).empty()) return Navigation::OpenExternal;
  return Navigation::Cancel;
}

}  // namespace wvp
