// The platform interface. Everything the AEGP side needs from a native web view host, with no SDK types and
// no platform headers. src/mac implements it with WKWebView; a Windows implementation (WebView2) goes in
// src/win with the same functions (see src/win/README.md). All functions run on After Effects' main thread.
#pragma once

#include <string>

namespace wvp::platform {

// The smallest size a panel window is allowed (points / device-independent pixels).
constexpr int kMinimumWidth = 260;
constexpr int kMinimumHeight = 320;

// Once, from the entry point after the panel is registered: process-wide setup (the macOS key monitor, WebKit
// defaults, theme observers). uninstall undoes it; it is called from the death hook.
void install();
void uninstall();

// From the create-panel hook. `nativeView` is the AEGP_PlatformViewRef After Effects hands over (an NSView*
// on macOS, an HWND on Windows). Adds a web view to it and starts loading the page. Returns the panel's
// refcon (given back to the functions below), or nullptr when the view could not be created. After Effects
// has no destroy callback for a panel, so the refcon is never released: it is a few dozen bytes per panel
// creation.
void* createPanel(void* nativeView);

// From the panel flyout menu.
void reloadPanel(void* refcon);
bool keyLoggingEnabled();
void setKeyLogging(bool enabled);

}  // namespace wvp::platform
