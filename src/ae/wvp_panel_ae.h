// Registers the panel with the Workspace Panel Suite: a Window menu command that toggles it and the hook that
// hands AE's native view to the platform layer. Modelled on the SDK's Panelator sample.
#pragma once

#include "wvp_sdk.h"

namespace wvp {

bool registerPanel(AEGP_PluginID pluginId);

}
