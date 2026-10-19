// The job queue implementation.
#include "wvp_queue.h"

#include <utility>

namespace wvp {

void JobQueue::push(Job job) {
  const std::lock_guard<std::mutex> lock(mutex_);
  jobs_.push_back(std::move(job));
}

std::optional<Job> JobQueue::pop() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (jobs_.empty()) return std::nullopt;
  Job job = std::move(jobs_.front());
  jobs_.pop_front();
  return job;
}

std::size_t JobQueue::size() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return jobs_.size();
}

}
