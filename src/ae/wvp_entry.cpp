// Plugin entry point plus the idle and death hooks. AEGP calls only work on AE's main thread inside a hook,
// so requests arrive as queued jobs and the idle hook runs them, for about 8 ms per call; on a failed start
// the plugin stays loaded but does nothing.
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

constexpr std::chrono::milliseconds kSliceBudget{8};

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
  if (plugin.queue.size() > 0 && wake != nullptr) wake();
}

A_Err idleHook(AEGP_GlobalRefcon, AEGP_IdleRefcon, A_long*) noexcept {
  try {
    if (state().running.load()) drainQueue();
  } catch (...) {
    wvp::logDroppedException("idle hook");
  }
  return A_Err_NONE;
}

A_Err deathHook(AEGP_GlobalRefcon, AEGP_DeathRefcon) noexcept {
  wvp::PluginState& plugin = state();
  plugin.running.store(false);
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
  plugin.causeIdle.store(plugin.suites.utility->AEGP_CauseIdleRoutinesToBeCalled);
  if (!registerHooks(plugin) || !wvp::registerPanel(pluginId)) return false;
  wvp::platform::install();
  plugin.running.store(true);
  return true;
}

}

extern "C" WVP_EXPORT A_Err EntryPointFunc(struct SPBasicSuite* basic, A_long majorVersion,
                                           A_long minorVersion, AEGP_PluginID pluginId,
                                           AEGP_GlobalRefcon* refcon) noexcept {
  if (refcon != nullptr) *refcon = nullptr;
  try {
    wvp::logLine(std::string("aegp-webview-panel ") + WVP_VERSION + " loading, AEGP API " +
                 std::to_string(majorVersion) + "." + std::to_string(minorVersion));
    if (!startUp(basic, pluginId)) wvp::logLine("start-up failed, the plugin stays inert");
  } catch (...) {
    wvp::logDroppedException("entry point");
  }
  return A_Err_NONE;
}
