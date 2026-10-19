// What platform code may ask of the AE side, with no SDK types. submitJob works from any thread and its reply
// runs on the idle hook; hostTheme may only be called from inside the idle hook.
#pragma once

#include <optional>

#include "wvp_queue.h"
#include "wvp_theme.h"

namespace wvp {

void submitJob(Job job);

std::optional<Theme> hostTheme();

}
