// The suites the plugin uses, acquired once on the main thread. Old suite versions are requested on purpose
// so the plugin loads on AE 25 and 26; ask for a newer version here if you need a newer function.
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
  const AEGP_UtilitySuite4* utility = nullptr;
  const AEGP_RegisterSuite5* registrar = nullptr;
  const AEGP_CommandSuite1* command = nullptr;
  const void* panel = nullptr;
  const PFAppSuite4* app = nullptr;
  std::vector<AcquiredSuite> acquired;
};

bool acquireSuites(const SPBasicSuite* basic, Suites& suites);

void releaseSuites(Suites& suites);

}
