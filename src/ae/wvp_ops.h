// The operations a page can ask for. Add your own in wvp_ops.cpp: a function that takes the argument text and
// returns an OpResult, plus one line in the table.
#pragma once

#include "wvp_queue.h"

namespace wvp {

// Main thread, inside the idle hook only. Never throws for an unknown operation: it returns an error result.
OpResult runOp(const Job& job);

}  // namespace wvp
