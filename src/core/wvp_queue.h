// The job queue between the threads that receive requests and the After Effects idle hook that runs them.
//
// Any thread may push (the web view's message handler, a worker thread, a socket thread...). Only the idle
// hook pops, because AEGP calls are only supported inside an AE hook on AE's main thread. See wvp_host.h and
// the README.
#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace wvp {

// The answer to one request: `json` is JSON text (a result when ok, an error message string when not).
struct OpResult {
  bool ok = false;
  std::string json = "null";
};

using Reply = std::function<void(const OpResult&)>;

// One request: the operation name, its argument text, and what to call with the answer (on the idle hook's
// thread).
struct Job {
  std::string op;
  std::string args;
  Reply reply;
};

class JobQueue {
 public:
  void push(Job job);
  std::optional<Job> pop();
  std::size_t size() const;

 private:
  mutable std::mutex mutex_;
  std::deque<Job> jobs_;
};

}  // namespace wvp
