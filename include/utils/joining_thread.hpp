#pragma once

#include <thread>
#include <utility>

namespace utils {

// RAII wrapper over std::thread: joins on destruction if still joinable.
// Intended for long-lived service workers — call join() during explicit
// shutdown when you need ordering; the destructor is the safety net.
// Not copyable; movable. Moved-from objects own no thread.
// Does not propagate exceptions from the worker (same as std::thread).

class joining_thread {
 public:
  joining_thread () noexcept = default;

  template <typename F, typename... Args>
  explicit joining_thread (F&& f, Args&&... args)
      : thread_ (std::forward<F> (f), std::forward<Args> (args)...) {}

  ~joining_thread () noexcept { join_if_joinable (); }

  joining_thread (const joining_thread& other) = delete;
  joining_thread& operator= (const joining_thread& other) = delete;

  joining_thread (joining_thread&& other) noexcept
      : thread_ {std::move (other.thread_)} {}

  joining_thread& operator= (joining_thread&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    join_if_joinable ();
    thread_ = std::move (other.thread_);
    return *this;
  }

  void join () {
    if (thread_.joinable ()) {
      thread_.join ();
    }
  }

  void detach () {
    if (thread_.joinable ()) {
      thread_.detach ();
    }
  }

  [[nodiscard]] bool joinable () const noexcept { return thread_.joinable (); }

  [[nodiscard]] std::thread::id get_id () const noexcept {
    return thread_.get_id ();
  }

  [[nodiscard]] std::thread::native_handle_type native_handle () {
    return thread_.native_handle ();
  }

 private:
  void join_if_joinable () noexcept {
    if (!thread_.joinable ()) {
      return;
    }

    try {
      thread_.join ();
    } catch (...) {
    }
  }

  std::thread thread_;
};

}  // namespace utils
