// The thread-safe job queue between whatever receives requests and the AE idle hook. Any thread may push;
// only the idle hook pops, because AEGP calls only work there.
#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace wvp {

struct OpResult {
  bool ok = false;
  std::string json = "null";
};

using Reply = std::function<void(const OpResult&)>;

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

}
