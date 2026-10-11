// The JavaScript the plugin gives every page, as text. Platform code installs it as user scripts / evaluates
// it.
#pragma once

#include <string>

namespace wvp {

// Document start, main frame only. Defines window.wvp:
//   wvp.call(op, args)  -> Promise: posts {id, op, args} to the native message handler "wvp"
//   wvp.theme           -> the host theme object (or null), updated before the "wvp-theme" event fires
//   wvp._resolve / wvp._setTheme -> called by the plugin only
// It also mirrors the theme into the CSS variables --wvp-bg, --wvp-fg and the color-scheme of the root
// element. `themeJson` is the initial theme as JSON text ("null" when AE would not say).
std::string bridgeShimScript(const std::string& themeJson);

// The script that completes the promise of request `id`. `json` is the JSON text of the result or the error
// message.
std::string replyScript(long long id, bool ok, const std::string& json);

// The script that tells the page the host theme changed.
std::string themeScript(const std::string& themeJson);

// Document end, every frame. Switches off spellcheck, autocorrect, autocomplete and writing suggestions on
// every input, textarea and contenteditable element, present or added later: WebKit's correction popups run a
// nested event loop that stalls After Effects' idle hook (README, "Gotchas").
std::string fieldHardeningScript();

}  // namespace wvp
