// Panel registration (AEGP Workspace Panel Suite): a Window menu command that toggles the panel, and the
// create-panel hook that hands After Effects' native view to the platform layer. Follows the SDK's Panelator
// sample.
#pragma once

#include "wvp_sdk.h"

namespace wvp {

// Main thread, from the entry point. False when registration failed (logged).
bool registerPanel(AEGP_PluginID pluginId);

}  // namespace wvp
