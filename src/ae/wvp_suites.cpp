#include "wvp_suites.h"

#include "wvp_log.h"

namespace wvp {

namespace {

// The Workspace Panel Suite, by value: its header cannot be included from plain C++ (wvp_sdk.h). The panel
// code (wvp_panel_ae.mm) checks the version against the SDK's kAEGPPanelSuiteVersion1 at compile time.
constexpr char kWvpPanelSuite[] = "AEGP Workspace Panel Suite";
constexpr int kWvpPanelSuiteVersion = 1;

template <typename Suite>
bool acquireOne(Suites& suites, const char* name, int version, const Suite*& slot) {
  const void* suite = nullptr;
  const auto error = suites.basic->AcquireSuite(name, version, &suite);
  if (error != 0 || suite == nullptr) {
    logLine(std::string("suite not available: ") + name + " v" + std::to_string(version) + " (SPErr " +
            std::to_string(error) + ")");
    return false;
  }
  slot = static_cast<const Suite*>(suite);
  suites.acquired.push_back({name, version});
  return true;
}

}  // namespace

bool acquireSuites(const SPBasicSuite* basic, Suites& suites) {
  suites.basic = basic;
  if (basic == nullptr) return false;
  const bool required =
      acquireOne(suites, kAEGPMemorySuite, kAEGPMemorySuiteVersion1, suites.memory) &&
      acquireOne(suites, kAEGPItemSuite, kAEGPItemSuiteVersion8, suites.item) &&
      acquireOne(suites, kAEGPCompSuite, kAEGPCompSuiteVersion8, suites.comp) &&
      acquireOne(suites, kAEGPUtilitySuite, kAEGPUtilitySuiteVersion4, suites.utility) &&
      acquireOne(suites, kAEGPRegisterSuite, kAEGPRegisterSuiteVersion5, suites.registrar) &&
      acquireOne(suites, kAEGPCommandSuite, kAEGPCommandSuiteVersion1, suites.command) &&
      acquireOne(suites, kWvpPanelSuite, kWvpPanelSuiteVersion, suites.panel);
  acquireOne(suites, kPFAppSuite, kPFAppSuiteVersion4, suites.app);
  return required;
}

void releaseSuites(Suites& suites) {
  if (suites.basic != nullptr) {
    for (const AcquiredSuite& suite : suites.acquired)
      suites.basic->ReleaseSuite(suite.name.c_str(), suite.version);
  }
  suites = Suites();
}

}  // namespace wvp
