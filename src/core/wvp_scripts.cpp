#include "wvp_scripts.h"

#include "wvp_json.h"

namespace wvp {

namespace {

const char* const kShimHead = R"JS((function () {
  'use strict';
  var pending = new Map();
  var nextId = 1;
  var wvp = { theme: null };
  wvp.call = function (op, args) {
    return new Promise(function (resolve, reject) {
      var id = nextId++;
      pending.set(id, { resolve: resolve, reject: reject });
      try {
        window.webkit.messageHandlers.wvp.postMessage({ id: id, op: String(op), args: args === undefined ? '' : String(args) });
      } catch (error) {
        pending.delete(id);
        reject(error);
      }
    });
  };
  wvp._resolve = function (id, ok, text) {
    var entry = pending.get(id);
    if (!entry) return;
    pending.delete(id);
    var value = JSON.parse(text);
    if (ok) entry.resolve(value);
    else entry.reject(new Error(String(value)));
  };
)JS";

const char* const kShimTail = R"JS(  function paint() {
    var root = document.documentElement;
    if (!root || !wvp.theme) return;
    root.style.setProperty('--wvp-bg', wvp.theme.background);
    root.style.setProperty('--wvp-fg', wvp.theme.dark ? '#e6e6e6' : '#1a1a1a');
    root.style.colorScheme = wvp.theme.dark ? 'dark' : 'light';
  }
  wvp._setTheme = function (theme) {
    wvp.theme = theme;
    paint();
    document.dispatchEvent(new CustomEvent('wvp-theme', { detail: theme }));
  };
  document.addEventListener('DOMContentLoaded', paint);
  window.wvp = wvp;
)JS";

}  // namespace

std::string bridgeShimScript(const std::string& themeJson) {
  return std::string(kShimHead) + kShimTail + "  wvp.theme = " + themeJson + ";\n})();\n";
}

std::string replyScript(long long id, bool ok, const std::string& json) {
  return "window.wvp._resolve(" + std::to_string(id) + "," + (ok ? "true" : "false") + "," + jsonQuote(json) +
         ");";
}

std::string themeScript(const std::string& themeJson) {
  return "window.wvp&&window.wvp._setTheme(" + themeJson + ");";
}

std::string fieldHardeningScript() {
  return R"JS((function () {
  'use strict';
  var selector = 'input, textarea, [contenteditable]';
  function harden(element) {
    element.setAttribute('spellcheck', 'false');
    element.setAttribute('autocorrect', 'off');
    element.setAttribute('autocomplete', 'off');
    element.setAttribute('autocapitalize', 'off');
    element.setAttribute('writingsuggestions', 'false');
  }
  function scan(root) {
    if (root.matches && root.matches(selector)) harden(root);
    if (root.querySelectorAll) root.querySelectorAll(selector).forEach(harden);
  }
  scan(document);
  new MutationObserver(function (records) {
    records.forEach(function (record) { record.addedNodes.forEach(function (node) { if (node.nodeType === 1) scan(node); }); });
  }).observe(document.documentElement, { childList: true, subtree: true });
})();
)JS";
}

}  // namespace wvp
