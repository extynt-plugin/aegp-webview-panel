// Which pages may use the bridge. The bridge can run ExtendScript, so it is only ever offered to the bundled
// page (a file URL inside the bundle's web folder) and, in debug builds only, to the development server's
// origin.
#pragma once

#include <string>

namespace wvp {

// "http://localhost:5173" for "http://localhost:5173/some/path?x=1"; "" when `url` is not an http(s) URL.
std::string originOf(const std::string& url);

// True for loopback http(s) origins only: localhost, 127.0.0.1 and [::1], with an optional port.
bool isLoopbackOrigin(const std::string& origin);

// True when `url` lies inside the bundle's web folder (`webPrefix`, a file URL ending in "/"; empty trusts no
// file at all) without climbing out of it, or has exactly the origin `devOrigin` (which may be empty). Any
// other file URL, such as a file dropped onto the panel, is not trusted.
bool isTrustedPageUrl(const std::string& url, const std::string& devOrigin, const std::string& webPrefix);

enum class Navigation { Allow, OpenExternal, Cancel };

// What the web view does with one navigation: trusted pages and about:blank load, a clicked http(s) link
// opens in the default browser, everything else (scripted redirects to other sites, custom schemes) is
// cancelled.
Navigation decideNavigation(const std::string& url, bool linkActivated, const std::string& devOrigin,
                            const std::string& webPrefix);

}  // namespace wvp
