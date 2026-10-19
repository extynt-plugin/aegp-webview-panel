// Page scripts. The shim defines wvp.call(op, args) (a Promise), wvp.theme and the --wvp-bg/--wvp-fg CSS
// variables; another script turns off spellcheck and autocorrect, whose popups would stall AE's idle hook.
#pragma once

#include <string>

namespace wvp {

std::string bridgeShimScript(const std::string& themeJson);

std::string replyScript(long long id, bool ok, const std::string& json);

std::string themeScript(const std::string& themeJson);

std::string fieldHardeningScript();

}
