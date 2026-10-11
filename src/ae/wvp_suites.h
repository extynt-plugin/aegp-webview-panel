// The AEGP suites the starter kit uses, acquired once on the main thread. Older suite versions are requested
// on purpose (After Effects keeps serving them, and they exist in every SDK from AE 25 on); to use a newer
// function, request the newer version here.
#pragma once

#include <string>
#include <vector>

#include "wvp_sdk.h"

namespace wvp {

struct AcquiredSuite {
  std::string name;
  int version = 0;
};

struct Suites {
  const SPBasicSuite* basic = nullptr;
  const AEGP_MemorySuite1* memory = nullptr;
  const AEGP_ItemSuite8* item = nullptr;
  const AEGP_CompSuite8* comp = nullptr;
  const AEGP_UtilitySuite4* utility = nullptr;  // v10: the oldest with AEGP_ExecuteScript
  const AEGP_RegisterSuite5* registrar = nullptr;
  const AEGP_CommandSuite1* command = nullptr;
  const void* panel =
      nullptr;  // AEGP_PanelSuite1 (a header only Objective-C++ can include): see wvp_panel_ae.mm
  const PFAppSuite4* app = nullptr;  // optional: without it the page gets no theme
  std::vector<AcquiredSuite> acquired;
};

// Main thread. False when a required suite is missing (each failure is logged); `suites` is then partly
// filled and must still go through releaseSuites.
bool acquireSuites(const SPBasicSuite* basic, Suites& suites);

// Main thread, from the death hook.
void releaseSuites(Suites& suites);

}  // namespace wvp
