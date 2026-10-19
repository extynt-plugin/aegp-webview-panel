// The interface between the AEGP side and a platform web view host, free of SDK and OS headers. src/mac
// implements it with WKWebView; a Windows port would add src/win. All of it runs on AE's main thread.
#pragma once

#include <string>

namespace wvp::platform {

constexpr int kMinimumWidth = 260;
constexpr int kMinimumHeight = 320;

void install();
void uninstall();

void* createPanel(void* nativeView);

void reloadPanel(void* refcon);
bool keyLoggingEnabled();
void setKeyLogging(bool enabled);

}
