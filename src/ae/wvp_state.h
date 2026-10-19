// Process-wide plugin state: the suites, the saved AEGP_CauseIdleRoutinesToBeCalled pointer (the one AEGP
// call that is safe from any thread) and the running and re-entrancy flags. Not for platform code.
#pragma once

#include <atomic>

#include "wvp_queue.h"
#include "wvp_sdk.h"
#include "wvp_suites.h"

namespace wvp {

using CauseIdleFn = A_Err (*)();

struct PluginState {
  AEGP_PluginID id = 0;
  Suites suites;
  JobQueue queue;
  std::atomic<CauseIdleFn> causeIdle{nullptr};
  std::atomic<bool> running{false};
  bool draining = false;
};

PluginState& state();

}
