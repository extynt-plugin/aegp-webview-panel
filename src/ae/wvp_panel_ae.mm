// Panel registration, flyout menu and Window menu hooks. Objective-C++ only because the SDK's panel header
// pulls in Cocoa; the six-parameter SDK hooks are variadic templates to stay within the five-parameter limit.
#include "wvp_panel_ae.h"

#include <string>
#include <tuple>

#include "wvp_log.h"
#include "wvp_platform.h"
#include "wvp_sdk_panels.h"
#include "wvp_state.h"

namespace wvp {

namespace {

static_assert(kAEGPPanelSuiteVersion1 == 1, "wvp_suites.cpp requests the panel suite by this version");

const A_u_char* const kMatchName = reinterpret_cast<const A_u_char*>("wvp-panel");
const char* const kMenuName = "WebView Panel";
const A_u_char* const kTitle = reinterpret_cast<const A_u_char*>("WebView Panel");
constexpr A_long kPreferredWidth = 360;
constexpr A_long kPreferredHeight = 640;

enum FlyoutCommand : AEGP_FlyoutMenuCmdID { kFlyoutReload = 31, kFlyoutLogKeys = 32 };

AEGP_Command g_menuCommand = 0;

void* viewToPointer(AEGP_PlatformViewRef view) { return (__bridge void*)view; }

const AEGP_PanelSuite1* panelSuite() { return static_cast<const AEGP_PanelSuite1*>(state().suites.panel); }

A_Err getSnapSizes(AEGP_PanelRefcon, A_LPoint* sizes, A_long* count) noexcept {
  sizes[0].x = platform::kMinimumWidth;
  sizes[0].y = platform::kMinimumHeight;
  sizes[1].x = kPreferredWidth;
  sizes[1].y = kPreferredHeight;
  *count = 2;
  return A_Err_NONE;
}

A_Err populateFlyout(AEGP_PanelRefcon, AEGP_FlyoutMenuItem* items, A_long* inOutCount) noexcept {
  const AEGP_FlyoutMenuMarkType keyMark =
      platform::keyLoggingEnabled() ? AEGP_FlyoutMenuMarkType_CHECKED : AEGP_FlyoutMenuMarkType_NORMAL;
  const AEGP_FlyoutMenuItem table[] = {
      {1, AEGP_FlyoutMenuMarkType_NORMAL, TRUE, kFlyoutReload,
       reinterpret_cast<const A_u_char*>("Reload page")},
      {1, keyMark, TRUE, kFlyoutLogKeys, reinterpret_cast<const A_u_char*>("Log keys (Console.app)")},
  };
  const A_long count = static_cast<A_long>(sizeof table / sizeof table[0]);
  for (A_long i = 0; i < count && i < *inOutCount; ++i) items[i] = table[i];
  *inOutCount = count;
  return A_Err_NONE;
}

A_Err doFlyoutCommand(AEGP_PanelRefcon refcon, AEGP_FlyoutMenuCmdID command) noexcept {
  try {
    if (command == kFlyoutReload) platform::reloadPanel(reinterpret_cast<void*>(refcon));
    if (command == kFlyoutLogKeys) platform::setKeyLogging(!platform::keyLoggingEnabled());
  } catch (...) {
    logDroppedException("flyout command");
  }
  return A_Err_NONE;
}

using CreateArgs = std::tuple<AEGP_GlobalRefcon, AEGP_CreatePanelRefcon, AEGP_PlatformViewRef, AEGP_PanelH,
                              AEGP_PanelFunctions1*, AEGP_PanelRefcon*>;
using CommandArgs =
    std::tuple<AEGP_GlobalRefcon, AEGP_CommandRefcon, AEGP_Command, AEGP_HookPriority, A_Boolean, A_Boolean*>;

void createPanel(const CreateArgs& args) {
  AEGP_PlatformViewRef container = std::get<2>(args);
  AEGP_PanelFunctions1* table = std::get<4>(args);
  table->GetSnapSizes = getSnapSizes;
  table->PopulateFlyout = populateFlyout;
  table->DoFlyoutCommand = doFlyoutCommand;
  if (container == nullptr) return;
  void* refcon = platform::createPanel(viewToPointer(container));
  *std::get<5>(args) = reinterpret_cast<AEGP_PanelRefcon>(refcon);
  panelSuite()->AEGP_SetTitle(std::get<3>(args), kTitle);
}

template <typename... Args>
A_Err createPanelHook(Args... args) noexcept {
  try {
    createPanel(CreateArgs(args...));
  } catch (...) {
    logDroppedException("create panel");
  }
  return A_Err_NONE;
}

template <typename... Args>
A_Err commandHook(Args... args) noexcept {
  const CommandArgs received(args...);
  if (std::get<2>(received) != g_menuCommand) return A_Err_NONE;
  panelSuite()->AEGP_ToggleVisibility(kMatchName);
  if (std::get<5>(received) != nullptr) *std::get<5>(received) = TRUE;
  return A_Err_NONE;
}

A_Err updateMenuHook(AEGP_GlobalRefcon, AEGP_UpdateMenuRefcon, AEGP_WindowType) noexcept {
  const Suites& suites = state().suites;
  suites.command->AEGP_EnableCommand(g_menuCommand);
  A_Boolean shown = FALSE;
  A_Boolean frontmost = FALSE;
  if (panelSuite()->AEGP_IsShown(kMatchName, &shown, &frontmost) == A_Err_NONE)
    suites.command->AEGP_CheckMarkMenuCommand(g_menuCommand, shown && frontmost);
  return A_Err_NONE;
}

}

bool registerPanel(AEGP_PluginID pluginId) {
  const Suites& suites = state().suites;
  A_Err err = suites.command->AEGP_GetUniqueCommand(&g_menuCommand);
  if (err == A_Err_NONE)
    err = suites.command->AEGP_InsertMenuCommand(g_menuCommand, kMenuName, AEGP_Menu_WINDOW,
                                                 AEGP_MENU_INSERT_SORTED);
  if (err == A_Err_NONE)
    err = suites.registrar->AEGP_RegisterCommandHook(pluginId, AEGP_HP_BeforeAE, g_menuCommand, commandHook,
                                                     nullptr);
  if (err == A_Err_NONE)
    err = suites.registrar->AEGP_RegisterUpdateMenuHook(pluginId, updateMenuHook, nullptr);
  if (err == A_Err_NONE)
    err = panelSuite()->AEGP_RegisterCreatePanelHook(pluginId, kMatchName, createPanelHook, nullptr, TRUE);
  if (err != A_Err_NONE) logLine("panel registration failed, AEGP error " + std::to_string(err));
  return err == A_Err_NONE;
}

}
