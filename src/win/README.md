# Windows (planned, not implemented)

Nothing in this directory builds yet. The layout is ready for it: everything outside `src/mac/` is meant to compile on
Windows unchanged, and `src/wvp_platform.h` is the whole interface a Windows port has to implement.

## The plan

- **WebView2** (Microsoft's Edge WebView2 runtime, the evergreen runtime ships with Windows 11 and most Windows 10
  machines) takes the place of WKWebView. Create the controller with `CreateCoreWebView2EnvironmentWithOptions` and
  `ICoreWebView2Environment::CreateCoreWebView2Controller` on the `HWND` After Effects passes to the panel hook
  (`AEGP_PlatformViewRef` is an `HWND` on Windows).
- **Bridge**: `ICoreWebView2::add_WebMessageReceived` replaces the `WKScriptMessageHandler`; the page posts with
  `window.chrome.webview.postMessage`. The shim in `src/core/wvp_scripts.cpp` needs a one-line change for that, and
  `ExecuteScript` replaces `evaluateJavaScript`. Everything after the message arrives (the job queue, the idle hook,
  the operations in `src/ae/`) is shared.
- **Threading** is the same rule as on macOS: the WebView2 callbacks run on the UI thread, outside any After Effects
  hook, so they only queue a job and wake the idle hook. See `src/ae/wvp_entry.cpp`.
- **Keyboard**: expect the same class of problem as the macOS event monitor (After Effects takes keys before a child
  window sees them). The likely tool is a `WH_GETMESSAGE`/`WH_KEYBOARD` hook or `ICoreWebView2Controller` accelerator-key
  handling (`add_AcceleratorKeyPressed`). The routing decisions in `src/mac/wvp_keys.cpp` are plain C++ and can move
  to `src/core/` when a second platform needs them.
- **Build**: a `cmake/plugin_win.cmake` next to `cmake/plugin_mac.cmake`, producing an `.aex`, with the PiPL compiled by
  Adobe's `PiPLtool`.

Contributions are welcome; open an issue first so the approach can be agreed.
