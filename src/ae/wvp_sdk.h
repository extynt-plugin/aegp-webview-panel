// The one place the Adobe SDK headers are included. They are found through AE_SDK_ROOT at build time and are
// never copied into this repository.
//
// AE_GeneralPlugPanels.h is NOT included here: on macOS it includes Cocoa and defines AEGP_PlatformViewRef as
// an NSView*, so only a file compiled as Objective-C++ can include it. wvp_panel_ae.mm is the only such file
// (CMake compiles it as OBJCXX), and it includes wvp_sdk_panels.h. Everything else stays plain C++.
#pragma once

#include "AEConfig.h"
#include "AE_GeneralPlug.h"
