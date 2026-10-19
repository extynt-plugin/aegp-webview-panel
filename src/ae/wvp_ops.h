// Dispatch for page operations. Runs on the idle hook only; an unknown operation returns an error result. To
// add one, write a function in wvp_ops.cpp and add a line to its table.
#pragma once

#include "wvp_queue.h"

namespace wvp {

OpResult runOp(const Job& job);

}
