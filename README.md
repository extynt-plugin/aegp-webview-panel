# aegp-webview-panel

A small, working starting point for an After Effects plugin whose UI is a web page. It is an AEGP plugin that adds a
dockable (and floatable) panel to After Effects, hosts a `WKWebView` in it, loads a bundled `index.html`, and gives
the page a message bridge to After Effects.

It exists because the part that is hard is not "put a web view in a panel". It is the keyboard, the event loop and the
threading: After Effects takes keys before a web view sees them, a modal popup anywhere in the process stalls the
plugin's idle hook, and AEGP calls made from the wrong place silently return wrong answers. Those fixes are the
point of this repository, and they are documented below so you can keep them when you replace the demo page.

The demo page shows the active composition's name (read through the AEGP suites) and has a button that runs
ExtendScript with `AEGP_ExecuteScript` and prints the result.

Status: version 0.1.0, macOS only. The platform-neutral parts are unit tested on Linux, and the plugin builds on
macOS against the After Effects 26.5 SDK, but its behaviour inside a running After Effects has not been confirmed
yet. If you have one, the checklist in
[docs/mac-test.md](docs/mac-test.md) is the most useful contribution you can make; see also "Known remaining issues"
below.

## Trademarks and third-party software

**extynt is an independent project. It is not affiliated with, endorsed by, sponsored by or approved by Adobe Inc. or any other company named in this section.**

- Adobe, After Effects and ExtendScript are either registered trademarks or trademarks of Adobe Inc. in the United States and/or other countries. AEGP, PiPL and the After Effects SDK are Adobe's plug-in interfaces and formats; they are named only to say what this code connects to.
- Apple, macOS, Xcode, Cocoa, WebKit and WKWebView are trademarks of Apple Inc., registered in the U.S. and other countries. Apple Silicon is an Apple product name.
- Microsoft, Windows and WebView2 are trademarks of the Microsoft group of companies.
- Google and Chrome are trademarks of Google LLC. The `.clang-format` file is based on Google's C++ style guide.
- Intel is a trademark of Intel Corporation. Linux is the registered trademark of Linus Torvalds. Ubuntu is a trademark of Canonical Ltd.
- Python is a trademark of the Python Software Foundation. CMake is a trademark of Kitware, Inc. LLVM, Clang, clang-format and clang-tidy are projects of the LLVM Foundation. GitHub is a trademark of GitHub, Inc.

All other trademarks are the property of their respective owners. References are for identification and interoperability only.

This repository does not include, and does not redistribute, any part of the Adobe After Effects SDK. To build the plugin you need your own copy of the SDK, obtained from Adobe under Adobe's license terms. The headers under `tests/syntax-check/ae-stubs` are independently written stand-ins for syntax checking only; they are not Adobe code.

Provided as is, without warranty. Use with Adobe software is subject to Adobe's own license terms.

The same text is in [NOTICE](NOTICE).

## Requirements

- macOS 12 or later (`@available` guards allow the newer WebKit APIs where present), Xcode with the command line
  tools, and CMake 3.20 or later.
- After Effects 25 or 26. The code asks for old, frozen suite versions on purpose, so it should load on both.
- The After Effects SDK. Adobe's licence does not allow redistributing it, so it is not in this repository. Download
  it from Adobe's developer site (search for "After Effects SDK", it is a free download behind an Adobe sign-in),
  unpack it anywhere, and point `AE_SDK_ROOT` at the unpacked folder. Either the folder that contains `Examples/` or
  the `Examples/` folder itself works.
- Optional: an Apple Developer ID certificate, to sign a build you give to other people.

## Build and install

```sh
export AE_SDK_ROOT=~/sdk/AfterEffectsSDK      # the unpacked SDK
./build-mac.sh                                # universal (arm64 + x86_64), ad-hoc signed
./build-mac.sh --install                      # also copies it into the After Effects Plug-ins folder
```

Options: `--arm64` (Apple Silicon only), `--debug-page` (Web Inspector and a development server, never ship that),
`--no-sign`, `--clean`, `--uninstall`, `--ae-year 2025` (default 2026, selects `/Applications/Adobe After Effects
<year>/Plug-ins`). Set `WVP_CODESIGN_ID` to a Developer ID identity to sign for distribution; without it the bundle is
ad-hoc signed, which is enough on the Mac that built it. A bundle copied to another Mac is quarantined by macOS:
run `xattr -cr wvpanel.plugin` there, or sign and notarize it.

Restart After Effects and open the panel from **Window > WebView Panel**.

By hand, without the script:

```sh
cmake -S . -B build -DAE_SDK_ROOT="$AE_SDK_ROOT" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build        # produces build/wvpanel.plugin
```

The PiPL (the resource After Effects reads to find the plugin) is `resources/wvpanel_PiPL.r`, written for this
project, and is compiled with Xcode's `Rez` by `scripts/rez-pipl.sh` as a post-build step. Before you ship your own
plugin, change the match name, the bundle identifier in `cmake/Info.plist.in`, and the PiPL name and category.

To work on the page: build with `--debug-page`, run a dev server, and put its URL (for example
`http://localhost:5173/`) on one line in `~/Library/Application Support/aegp-webview-panel/dev-url.txt`. Choose
"Reload page" in the panel's flyout menu. Release builds never read that file, because the bridge can run
ExtendScript and must only be reachable from the bundled page.

## Architecture

```
  web page (web/index.html)                       After Effects process
  ─────────────────────────                       ─────────────────────────────────────────────────────
  wvp.call("comp.active_name")                   main thread
        │  window.webkit.messageHandlers.wvp        │
        ▼                                           │
  WKWebView ──────────────► WvpPanel (wvp_bridge.mm)│  trust check (bundle's web/ folder or dev origin only)
  (src/mac/wvp_panel*.mm)         │  builds a Job   │
                                  ▼                 │
                        JobQueue (thread safe) ◄────┘  submitJob() also calls
                                  │                    AEGP_CauseIdleRoutinesToBeCalled to wake AE
                                  ▼
                 AE idle hook (src/ae/wvp_entry.cpp)   ◄── AE calls this ~30 times a second, ON the main thread,
                                  │                        INSIDE a hook: the only place AEGP calls are valid
                                  ▼
                 runOp (src/ae/wvp_ops.cpp)  ── AEGP suites: comp, item, memory, utility (ExecuteScript)
                                  │
                                  ▼
                 Job.reply ──► evaluateJavaScript("wvp._resolve(...)") ──► the promise in the page resolves

  keyboard:  NSEvent local monitor (src/mac/wvp_monitor.mm) ── decides per key (src/mac/wvp_keys.cpp)
             whether the web view or After Effects gets it
  panel:     AEGP Workspace Panel Suite (src/ae/wvp_panel_ae.mm) gives a container NSView; we add the web view to it
```

Layout: `src/core/` is plain C++ with no OS or SDK dependency (JSON quoting, the theme, the job queue, the page
scripts, the URL trust rules) and is unit tested. `src/ae/` is everything that calls the Adobe SDK. `src/mac/` is
Objective-C++. `src/wvp_platform.h` is the whole interface between the AEGP side and a platform, so a Windows port
adds `src/win/` without touching the rest.

### Why work runs on the idle hook

AEGP suites are only supported on After Effects' main thread while it is inside one of its callbacks (a hook). A
`WKScriptMessageHandler`, a `dispatch_async` block or a timer also run on the main thread, but outside any hook, and
the suites misbehave there: for example `AEGP_GetMostRecentlyUsedComp` reports no project. So the page's message is
turned into a `Job` and queued, and the idle hook, which After Effects calls roughly 30 times a second, runs it and
sends the reply back to the page.

The queue is thread safe because jobs can come from any thread (a network callback, a worker). To avoid waiting for
the next idle tick, the code saves the pointer to `AEGP_CauseIdleRoutinesToBeCalled` during start-up (on the main
thread, where the suites are acquired) and calls it after each push. That is the one AEGP function Adobe documents as
callable from other threads. The idle hook runs jobs for at most about 8 ms per call and asks for another call if more
is waiting, so a burst of requests cannot freeze the interface.

## Keyboard and idle-loop gotchas

Each of these is handled in the code.

- **After Effects takes keys first.** In a docked panel Cmd+A selects all layers, the arrows move the time cursor,
  Delete edits the project, and Cmd+C/V/X/Z do nothing in the page. An `NSEvent` local monitor
  (`src/mac/wvp_monitor.mm`) runs before the key reaches a window or a menu key equivalent, and routes it: when the
  first responder is inside a web view, editing shortcuts are sent as AppKit actions straight to the responder inside
  the web view (never to the window's whole chain, which would reach After Effects), other plain keys are delivered with
  `keyDown:`/`keyUp:` so typing and input methods work, and the event is consumed. Cmd/Ctrl combinations that are not
  editing (Cmd+S, Cmd+Q) go to After Effects. The routing table is `src/mac/wvp_keys.cpp`, unit tested.
- **WebKit hands a key back.** A key WebKit does not use is sent through `[NSApp sendEvent:]` again, so the monitor
  sees it twice and After Effects would act on the second copy. Recently routed events are remembered and the repeat
  is swallowed.
- **Focus must be handed back.** A click on a web view makes it first responder (a floating panel is not even key until
  clicked, so the first click is also accepted as a click). A click anywhere else in a window whose first responder is
  still inside a web view returns focus to what had it, or Delete would keep going to the page.
- **Floating panels.** A floating panel's window has no minimum size and can be dragged to a sliver; a minimum content
  size is set on any window that is not After Effects' main window.
- **Modal and nested loops stall the idle hook.** WebKit's autocorrect, text-replacement and spelling popups,
  JavaScript `alert`/`confirm`/`prompt`, and file pickers all run a nested event loop on the main thread. While one is
  open, After Effects does not call the idle hook, so queued work waits and a call from another thread looks hung.
  The fixes: the `WKUIDelegate` answers dialogs and file pickers immediately without showing them; the `Web*`
  spelling/substitution defaults are registered as off in memory only (nothing is written to After Effects'
  preferences); and a script injected into every page switches off `spellcheck`, `autocorrect`, `autocomplete`,
  `autocapitalize` and writing suggestions on every field, including ones added later. If your page needs a file
  picker, do it in native code in a way that does not block the idle hook.
- **Errors can open dialogs too.** A failing AEGP call can raise After Effects' own modal error dialog; operations run
  inside `AEGP_StartQuietErrors`/`EndQuietErrors` for that reason.
- **Do not let exceptions out.** Every callback from Cocoa, WebKit and After Effects is wrapped so nothing is thrown
  across the boundary.
- **Theme.** The page gets After Effects' background colour (`PF_AppGetBgColor`) as CSS variables and a `wvp-theme`
  event. After Effects announces no brightness change, so the colour is re-read when a window becomes key or the
  application becomes active (which covers closing Preferences). The read is queued as a job (`host.theme`) and runs on
  the idle hook like every other host call.
- **Only the bundled page may use the bridge.** The bridge can run ExtendScript, so a page is trusted only if its URL is
  inside the bundle's `web/` folder (`..` and percent-encoded separators are refused), or, in debug builds, has the
  origin of a development server on localhost, 127.0.0.1 or [::1]. A file dropped onto the panel is not trusted and
  is not allowed to navigate the panel. Answers carry the page load that asked, so a reply that arrives after a reload
  is dropped instead of resolving the wrong promise.

### Known remaining issues

Reported by early testing, not yet fixed here. Check them on your Mac before relying on the behaviour.

- **Home / End:** WebKit sends these back through the event stream after the page handled them, and in some situations
  the repeat is not recognised, so After Effects may also react (moving the time cursor). The ring that detects
  repeats may be too short for a key held down.
- **Escape:** the monitor does not see Escape reliably while a field has focus, so the page may not get it; After
  Effects may handle it first.
- **Floating panel minimum size:** After Effects may override the window's minimum size.
- Paste has no JavaScript fallback (browsers allow clipboard reads only on a user gesture), so Cmd+V relies on the
  AppKit `paste:` action reaching the web view.

## Not done yet

- Windows: WebView2 in place of WKWebView. The plan is in [src/win/README.md](src/win/README.md); the platform
  interface and the shared code are already separated for it.
- A signed and notarized release build.

## Checks

```sh
tests/syntax-check/check-syntax.sh --tidy   # compiles every file against stub headers, then clang-tidy
scripts/check-limits.sh                     # file, function, complexity, nesting and parameter limits (lizard)
cmake -S . -B build -DWVP_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build
```

The syntax check needs no macOS and no SDK: Cocoa, WebKit and the Adobe headers are replaced by small declarations in
`tests/syntax-check/`. If `AE_SDK_ROOT` is set it uses the real SDK headers instead. A pass means the code parses and
type-checks; it does not prove it works in After Effects. CI (`.github/workflows/ci.yml`) runs all of the above plus
a `clang-format` check.

The coding limits hold for every file, scripts included, with no suppression comments: a file is at most 500 lines, a
function at most 80 lines, cyclomatic complexity at most 10, nesting at most 4, and at most 5 parameters. The
Adobe SDK's six-parameter callbacks are written as variadic templates so they stay within the limit.

## Licence

MIT, see [LICENSE](LICENSE). The Adobe After Effects SDK is not part of this repository and has its own licence.

Copyright 2026 extynt. Built by the team behind extynt (https://extynt.com), an After Effects copilot.
