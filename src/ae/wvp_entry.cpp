// Plugin entry point and the two AEGP hooks the kit needs: the idle hook (runs queued work) and the death
// hook.
//
// Threading in one paragraph. AEGP calls are only supported on After Effects' main thread, inside one of its
// hooks (calling the same suites from a run-loop block, a timer or a WKScriptMessageHandler returns wrong
// answers such as "no project"). So nothing here calls an AEGP suite outside a hook. Work arrives as jobs in
// a thread-safe queue (wvp_queue.h); the idle hook, which AE calls about 30 times a second, pops and runs
// them. After a push, the saved AEGP_CauseIdleRoutinesToBeCalled pointer asks AE to run the idle hook sooner:
// it is the one AEGP function the SDK documents as safe to call from another thread, and its pointer has to
// be fetched on the main thread (below).
#include <chrono>
#include <string>
#include <utility>

#include "wvp_json.h"
#include "wvp_log.h"
#include "wvp_ops.h"
#include "wvp_panel_ae.h"
#include "wvp_platform.h"
#include "wvp_state.h"

#ifndef WVP_VERSION
#define WVP_VERSION "0.0.0-dev"
#endif

#define WVP_EXPORT __attribute__((visibility("default")))

namespace {

using wvp::state;

constexpr std::chrono::milliseconds kSliceBudget{
    8};  // time spent on jobs per idle call (one job always runs)

// Runs one job and answers it. Nothing thrown by an operation or a reply may reach After Effects.
void runJob(wvp::Job& job) {
  wvp::OpResult result;
  try {
    result = wvp::runOp(job);
  } catch (...) {
    wvp::logDroppedException("operation");
    result = wvp::OpResult{false, wvp::jsonQuote("internal error")};
  }
  try {
    if (job.reply) job.reply(result);
  } catch (...) {
    wvp::logDroppedException("reply");
  }
}

// Main thread, inside the idle hook. The guard stops a nested idle call (an AEGP call can pump the idle loop)
// from running jobs inside jobs.
void drainQueue() {
  wvp::PluginState& plugin = state();
  if (plugin.draining) return;
  plugin.draining = true;
  const auto start = std::chrono::steady_clock::now();
  while (std::optional<wvp::Job> job = plugin.queue.pop()) {
    runJob(*job);
    if (std::chrono::steady_clock::now() - start > kSliceBudget) break;
  }
  plugin.draining = false;
  const wvp::CauseIdleFn wake = plugin.causeIdle.load();
  if (plugin.queue.size() > 0 && wake != nullptr)
    wake();  // more is waiting: do not leave it for the next 30 Hz tick
}

A_Err idleHook(AEGP_GlobalRefcon, AEGP_IdleRefcon, A_long*) noexcept {
  try {
    if (state().running.load()) drainQueue();
  } catch (...) {
    wvp::logDroppedException("idle hook");  // the next idle call tries again
  }
  return A_Err_NONE;
}

A_Err deathHook(AEGP_GlobalRefcon, AEGP_DeathRefcon) noexcept {
  wvp::PluginState& plugin = state();
  plugin.running.store(false);  // from here the idle hook does nothing and submitJob refuses new work
  plugin.causeIdle.store(nullptr);
  try {
    wvp::platform::uninstall();
    wvp::releaseSuites(plugin.suites);
  } catch (...) {
    wvp::logDroppedException("death hook");
  }
  return A_Err_NONE;
}

bool registerHooks(wvp::PluginState& plugin) {
  const wvp::Suites& suites = plugin.suites;
  const A_Err idle = suites.registrar->AEGP_RegisterIdleHook(plugin.id, idleHook, nullptr);
  const A_Err death = suites.registrar->AEGP_RegisterDeathHook(plugin.id, deathHook, nullptr);
  if (idle != A_Err_NONE || death != A_Err_NONE) {
    wvp::logLine("hook registration failed: idle " + std::to_string(idle) + ", death " +
                 std::to_string(death));
    return false;
  }
  return true;
}

bool startUp(const SPBasicSuite* basic, AEGP_PluginID pluginId) {
  wvp::PluginState& plugin = state();
  plugin.id = pluginId;
  if (!wvp::acquireSuites(basic, plugin.suites)) return false;
  plugin.causeIdle.store(
      plugin.suites.utility->AEGP_CauseIdleRoutinesToBeCalled);  // main thread, as the SDK asks
  if (!registerHooks(plugin) || !wvp::registerPanel(pluginId)) return false;
  wvp::platform::install();
  plugin.running.store(true);
  return true;
}

}  // namespace

extern "C" WVP_EXPORT A_Err EntryPointFunc(struct SPBasicSuite* basic, A_long majorVersion,
                                           A_long minorVersion, AEGP_PluginID pluginId,
                                           AEGP_GlobalRefcon* refcon) noexcept {
  if (refcon != nullptr) *refcon = nullptr;
  try {
    wvp::logLine(std::string("aegp-webview-panel ") + WVP_VERSION + " loading, AEGP API " +
                 std::to_string(majorVersion) + "." + std::to_string(minorVersion));
    // On failure the plugin stays loaded but inert: running stays false, so no hook does any work. Suites
    // already acquired are released by the death hook (when it was registered) or by process exit.
    if (!startUp(basic, pluginId)) wvp::logLine("start-up failed, the plugin stays inert");
  } catch (...) {
    wvp::logDroppedException("entry point");
  }
  return A_Err_NONE;
}
