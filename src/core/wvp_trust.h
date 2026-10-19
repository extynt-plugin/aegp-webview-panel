// Which pages may use the bridge, which can run ExtendScript: files in the bundle's web folder and, in debug
// builds, a loopback dev server. Clicked http(s) links open in the browser; other navigations are cancelled.
#pragma once

#include <string>

namespace wvp {

std::string originOf(const std::string& url);

bool isLoopbackOrigin(const std::string& origin);

bool isTrustedPageUrl(const std::string& url, const std::string& devOrigin, const std::string& webPrefix);

enum class Navigation { Allow, OpenExternal, Cancel };

Navigation decideNavigation(const std::string& url, bool linkActivated, const std::string& devOrigin,
                            const std::string& webPrefix);

}
