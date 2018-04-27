// Copyright (c) 2014-2017 Agora.io, Inc.
// A block queue implements a multithreaded event queue.
//

#pragma once  // NOLINT(build/header_guard)

#include <condition_variable>  // NOLINT(build/c++11)
#include <deque>
#include <mutex>  // NOLINT(build/c++11)
#include <utility>

template <typename T>
class BlockQueue {
 public:
  BlockQueue() : mutex_(), notEmpty_(), queue_() {}

  void put(const T &x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify_one();
  }

  void put(T &&x) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(x));
    notEmpty_.notify_one();
  }

  T take() {
    std::unique_lock<std::mutex> lk(mutex_);

    // always use a while-loop, due to spurious wakeup
    while (queue_.empty()) {
      notEmpty_.wait(lk);
    }
    T front((queue_.front()));
    queue_.pop_front();
    return front;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable notEmpty_;
  std::deque<T> queue_;
};
