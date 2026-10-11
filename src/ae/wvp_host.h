// What platform code (src/mac, src/win) may ask of the AE side. No SDK types, so platform code stays
// SDK-free.
#pragma once

#include <optional>

#include "wvp_queue.h"
#include "wvp_theme.h"

namespace wvp {

// Any thread. Queues `job` for the idle hook and asks After Effects to call the idle hook soon. The job's
// reply is called on After Effects' main thread, from inside the idle hook (or, when the plugin is not
// running, at once on the calling thread with an error).
void submitJob(Job job);

// Idle hook only (it calls a PF suite, which is unsupported outside a hook): AE's UI background colour, or
// nothing when the suite is missing or the call fails.
std::optional<Theme> hostTheme();

}  // namespace wvp
