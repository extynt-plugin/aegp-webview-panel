// The plugin's process-wide state. SDK types live here; platform code never includes this file (it uses
// wvp_host.h).
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
  // AEGP_CauseIdleRoutinesToBeCalled, fetched on the main thread at startup: the one AEGP function that is
  // safe to call from any other thread (the SDK says so), and the way to make AE run the idle hook soon.
  std::atomic<CauseIdleFn> causeIdle{nullptr};
  std::atomic<bool> running{false};  // false before startup finished and after the death hook began
  bool draining = false;             // main thread only: the idle hook is running jobs (re-entrancy guard)
};

PluginState& state();

}  // namespace wvp
