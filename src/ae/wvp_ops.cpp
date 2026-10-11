#include "wvp_ops.h"

#include <optional>
#include <string>

#include "wvp_host.h"
#include "wvp_json.h"
#include "wvp_state.h"
#include "wvp_theme.h"
#include "wvp_utf.h"

namespace wvp {

namespace {

OpResult failure(const std::string& message) { return OpResult{false, jsonQuote(message)}; }
OpResult success(const std::string& json) { return OpResult{true, json}; }

// Quiets AE's own error dialogs for the AEGP calls in scope: a failing call would otherwise open a modal
// dialog, and a modal dialog on AE's main thread throttles the idle hook (README, "Gotchas").
class QuietErrors {
 public:
  explicit QuietErrors(const AEGP_UtilitySuite4* utility) : utility_(utility) {
    if (utility_ != nullptr) active_ = utility_->AEGP_StartQuietErrors(&token_) == A_Err_NONE;
  }
  ~QuietErrors() {
    if (active_) utility_->AEGP_EndQuietErrors(FALSE, &token_);
  }
  QuietErrors(const QuietErrors&) = delete;
  QuietErrors& operator=(const QuietErrors&) = delete;

 private:
  const AEGP_UtilitySuite4* utility_;
  AEGP_ErrReportState token_{};
  bool active_ = false;
};

// Copies the bytes of a memory handle and frees the handle. `wide` handles hold null-terminated UTF-16.
std::string takeHandle(const AEGP_MemorySuite1* memory, AEGP_MemHandle handle, bool wide) {
  if (handle == nullptr) return "";
  void* data = nullptr;
  std::string text;
  if (memory->AEGP_LockMemHandle(handle, &data) == A_Err_NONE && data != nullptr) {
    text =
        wide ? utf16ToUtf8(static_cast<const char16_t*>(data)) : std::string(static_cast<const char*>(data));
    memory->AEGP_UnlockMemHandle(handle);
  }
  memory->AEGP_FreeMemHandle(handle);
  return text;
}

// comp.active_name: the name of the composition AE reports as most recently used, or null when there is none.
OpResult activeCompName(const std::string& /*args*/) {
  PluginState& plugin = state();
  const Suites& suites = plugin.suites;
  const QuietErrors quiet(suites.utility);
  AEGP_CompH comp = nullptr;
  if (suites.comp->AEGP_GetMostRecentlyUsedComp(&comp) != A_Err_NONE)
    return failure("could not read the active composition");
  if (comp == nullptr) return success("null");
  AEGP_ItemH item = nullptr;
  if (suites.comp->AEGP_GetItemFromComp(comp, &item) != A_Err_NONE)
    return failure("could not get the composition's item");
  AEGP_MemHandle name = nullptr;
  if (suites.item->AEGP_GetItemName(plugin.id, item, &name) != A_Err_NONE)
    return failure("could not read the item name");
  return success(jsonQuote(takeHandle(suites.memory, name, true)));
}

// script.run: runs the argument as ExtendScript and returns what the script's last expression evaluates to,
// as a string. platform_encodingB is FALSE, so strings are UTF-8 (docs/mac-test.md checks a non-ASCII round
// trip).
OpResult runScript(const std::string& script) {
  PluginState& plugin = state();
  const Suites& suites = plugin.suites;
  AEGP_MemHandle result = nullptr;
  AEGP_MemHandle error = nullptr;
  const A_Err status = suites.utility->AEGP_ExecuteScript(plugin.id, script.c_str(), FALSE, &result, &error);
  const std::string resultText = takeHandle(suites.memory, result, false);
  const std::string errorText = takeHandle(suites.memory, error, false);
  if (status != A_Err_NONE)
    return failure("AEGP_ExecuteScript failed (error " + std::to_string(status) + ")");
  if (!errorText.empty()) return failure(errorText);
  return success(jsonQuote(resultText));
}

// host.theme: After Effects' UI background colour as theme JSON, or null. Run on the idle hook because
// PF_AppGetBgColor, like every host call, is only supported inside a hook.
OpResult hostThemeOp(const std::string& /*args*/) {
  const std::optional<Theme> theme = hostTheme();
  return success(theme ? themeJson(*theme) : "null");
}

struct OpEntry {
  const char* name;
  OpResult (*run)(const std::string& args);
};

constexpr OpEntry kOps[] = {
    {"comp.active_name", activeCompName},
    {"script.run", runScript},
    {"host.theme", hostThemeOp},
};

}  // namespace

OpResult runOp(const Job& job) {
  for (const OpEntry& entry : kOps) {
    if (job.op == entry.name) return entry.run(job.args);
  }
  return failure("unknown operation: " + job.op);
}

}  // namespace wvp
