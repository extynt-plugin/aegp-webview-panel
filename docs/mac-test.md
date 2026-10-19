# Contributing: macOS test checklist

Help wanted from anyone with a Mac, After Effects 25 or 26 and Xcode. The plugin builds on macOS against the
After Effects 26.5 SDK, but it has not been exercised in a running After Effects. Please open an issue with each line as pass, fail (with what you
saw), or not tried. The most useful extra is a Console.app log: filter on `[wvp]`.

## 1. Build

1. Download and unpack the After Effects SDK (see the README). `export AE_SDK_ROOT=/path/to/AfterEffectsSDK`
2. `./build-mac.sh --clean`. Expect `built .../build/wvpanel.plugin`. Failure here: send the last 30 lines.
3. Optional: `./build-mac.sh --arm64 --debug-page` to check the other configurations compile.
4. `lipo -archs build/wvpanel.plugin/Contents/MacOS/wvpanel` shows `x86_64 arm64` for the universal build.
5. `ls build/wvpanel.plugin/Contents/Resources` shows `wvpanel.rsrc` (not empty) and `web/index.html`. A missing or
   empty `.rsrc` means the Rez step failed: run `WVP_REZ_VERBOSE=1 ./build-mac.sh` and send the output.

## 2. Install and load

1. Quit After Effects. `./build-mac.sh --install --ae-year 2026` (use your version's year).
2. Start After Effects. Console.app, filter `[wvp]`: expect "aegp-webview-panel 0.1.0 loading" and no
   "start-up failed" or "suite not available" lines. Note which suites, if any, were missing.
3. **Window > WebView Panel** opens the panel with the demo page ("WebView Panel" heading, three sections).

## 3. Dock and float

1. Drag the panel's tab into a frame next to the Project panel: it docks and the page fills it.
2. Resize the frame: the page follows without gaps or a stale area.
3. Undock it (panel menu or drag out) to a floating window: the page still renders. Drag it to the smallest size:
   it should stop at roughly 260 x 320 points (report if you can shrink it further).
4. Close and reopen it from the Window menu 5 times, docked and floating: no crash, no blank page.
5. Panel flyout menu: "Reload page" reloads; "Log keys (Console.app)" toggles a check mark.
6. Change After Effects' brightness in Preferences > Appearance, close Preferences: the page background follows
   within a second or so (it is re-read when the app becomes active).

## 4. Bridge

1. Open a project with at least one comp, make a comp active. Click **Refresh**: its exact name appears.
   With no project open the field shows "(no composition)".
2. In the ExtendScript box keep the default and click **Run**: it prints "N items in the project".
3. UTF-8 round trip: set the script to `"héllo ✓ 日本語"` and run: the output is identical, not garbled.
4. A script error: run `nonexistent()`: an error message appears in the page, with no After Effects dialog.

## 5. Keyboard matrix

Use the "Keyboard check" text area. Turn on "Log keys" in the flyout to see each decision in Console.app. Set it up
docked first, then repeat everything floating. Between rows click a layer in the timeline to be sure focus returns.

| Action in the text area                          | Expected                                        | Docked | Floating |
| ------------------------------------------------ | ----------------------------------------------- | ------ | -------- |
| Type text, including accented keys (Option+E, e) | Appears in the page; nothing changes in AE      |        |          |
| Cmd+A                                            | Selects the text; AE does not select all layers |        |          |
| Cmd+C then Cmd+V elsewhere in the area           | Copies and pastes the text                      |        |          |
| Cmd+X                                            | Cuts the selection                              |        |          |
| Cmd+Z, Shift+Cmd+Z                               | Undo and redo typing; AE undo stack untouched   |        |          |
| Left / Right / Up / Down                         | Move the caret; the time cursor stays put       |        |          |
| Shift+arrows, Option+arrows, Cmd+arrows          | Select / move by word / by line                 |        |          |
| Backspace, Forward Delete                        | Delete text; no layer or keyframe is deleted    |        |          |
| Home, End, Shift+Home, Shift+End                 | Caret to line start/end; AE time cursor stays   |        |          |
| Page Up / Page Down                              | Scroll the area; AE time cursor stays           |        |          |
| Escape                                           | Report what happens (known issue)               |        |          |
| Cmd+S                                            | AE saves the project (passed through)           |        |          |
| Click the timeline, then press Delete            | AE deletes the selected layer (focus handed back) |      |          |
| Held key (hold a letter for 2 s)                 | Repeats in the page only                        |        |          |

## 6. Idle-stall check

This checks that nothing in the page can freeze the bridge.

1. In the keyboard area type a misspelt word such as `teh ` and a straight quote `"`: no red underline, no
   autocorrection, no smart-quote substitution, no popup.
2. Right-click in the page: report what menu appears (a native menu is fine, a long freeze is not).
3. Drag an `.html` file from Finder onto the panel: nothing may happen (the file must not load, and a script on
   it must not be able to reach the bridge).
4. Open the page's Web Inspector (debug build only: right-click > Inspect Element), run `alert("x")` and
   `confirm("x")` in its console: no dialog appears, the call returns at once.
5. With the Inspector open, run `setInterval(() => wvp.call('comp.active_name'), 100)` and keep typing in the
   keyboard area for 30 seconds: the comp name keeps updating in the page and After Effects stays responsive.
6. Console.app should show no "idle" or "exception dropped" lines. Report any.

## 7. Report

For each failure: the section and row, the After Effects version, macOS version, chip (Apple Silicon or Intel), and
the `[wvp]` log lines from Console.app around the time.
