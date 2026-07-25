#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <deque>
#include <optional>
#include <stdexcept>
#include <utility>

namespace utils {
// Fixed-duration buffer storing (timestamp, value) samples in insertion order.
//
// Calling push(t, v) appends a sample and removes any samples with timestamps
// strictly older than t - window(). All queries operate on the window anchored
// at the timestamp of the most recent push.
//
// ready() returns true once the buffer spans at least one full window, i.e.
// latest_time() - oldest_time() >= window(). An empty buffer is never ready.
//
// Timestamps are expected to be sufficiently monotonic for the caller's use
// case. Samples are never reordered: out-of-order pushes are stored as
// received, and pruning is always based on the latest pushed timestamp.
// Fixed-duration buffer of (timestamp, value) samples in insertion order.
template <typename T, typename Clock = std::chrono::steady_clock>
class rolling_time_window {
 public:
  using clock = Clock;
  using time_point = typename Clock::time_point;
  using duration = typename Clock::duration;
  using sample = std::pair<time_point, T>;

  explicit rolling_time_window (duration window) : window_ {window} {
    if (window_ <= duration::zero ()) {
      throw std::invalid_argument ("window duration must be positive");
    }
  }

  void push (time_point t, const T& value) { push_impl (t, value); }

  void push (time_point t, T&& value) { push_impl (t, std::move (value)); }

  void clear () noexcept { samples_.clear (); }

  [[nodiscard]] duration window () const noexcept { return window_; }

  [[nodiscard]] bool empty () const noexcept { return samples_.empty (); }

  [[nodiscard]] std::size_t size () const noexcept { return samples_.size (); }

  [[nodiscard]] std::optional<T> latest_value () const
    requires std::copy_constructible<T>
  {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.back ().second;
  }

  [[nodiscard]] std::optional<T> oldest_value () const
    requires std::copy_constructible<T>
  {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.front ().second;
  }

  [[nodiscard]] std::optional<time_point> latest_time () const {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.back ().first;
  }

  [[nodiscard]] std::optional<time_point> oldest_time () const {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.front ().first;
  }

  // True when the buffer spans at least window() from oldest to latest sample.
  [[nodiscard]] bool ready () const noexcept {
    if (samples_.empty ()) {
      return false;
    }
    return (samples_.back ().first - samples_.front ().first) >= window_;
  }

  [[nodiscard]] duration span () const {
    if (samples_.empty ()) {
      return duration::zero ();
    }
    return samples_.back ().first - samples_.front ().first;
  }

 private:
  template <typename U>
  void push_impl (time_point t, U&& value) {
    samples_.emplace_back (t, std::forward<U> (value));
    prune_older_than (t - window_);
  }

  void prune_older_than (time_point cutoff) {
    while (!samples_.empty () && samples_.front ().first < cutoff) {
      samples_.pop_front ();
    }
  }

  duration window_;
  std::deque<sample> samples_;
};

}  // namespace utils
