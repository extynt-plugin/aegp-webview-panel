#include "wvp_host.h"

#include <utility>

#include "wvp_state.h"

namespace wvp {

PluginState& state() {
  static PluginState instance;
  return instance;
}

void submitJob(Job job) {
  PluginState& plugin = state();
  if (!plugin.running.load()) {
    if (job.reply) job.reply(OpResult{false, "\"the plugin is not running\""});
    return;
  }
  plugin.queue.push(std::move(job));
  const CauseIdleFn wake = plugin.causeIdle.load();
  if (wake != nullptr) wake();
}

std::optional<Theme> hostTheme() {
  const PFAppSuite4* app = state().suites.app;
  if (app == nullptr || app->PF_AppGetBgColor == nullptr) return std::nullopt;
  PF_App_Color color = {0, 0, 0};
  if (app->PF_AppGetBgColor(&color) != PF_Err_NONE) return std::nullopt;
  return themeFromChannels16(color.red, color.green, color.blue);
}

}  // namespace wvp
