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
  using time_point = typename clock::time_point;
  using duration = typename clock::duration;
  using sample = std::pair<time_point, T>;

  explicit rolling_time_window (duration window) : window_ {window} {
    if (window_ <= duration::zero ()) {
      throw std::invalid_argument ("window duration must be positive");
    }
  }

  auto push (time_point t, const T& value) -> void { push_impl (t, value); }

  auto push (time_point t, T&& value) -> void {
    push_impl (t, std::move (value));
  }

  auto clear () noexcept -> void { samples_.clear (); }

  [[nodiscard]] auto window () const noexcept -> duration { return window_; }

  [[nodiscard]] auto empty () const noexcept -> bool {
    return samples_.empty ();
  }

  [[nodiscard]] auto size () const noexcept -> std::size_t {
    return samples_.size ();
  }

  [[nodiscard]] auto latest_value () const -> std::optional<T>
    requires std::copy_constructible<T>
  {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.back ().second;
  }

  [[nodiscard]] auto oldest_value () const -> std::optional<T>
    requires std::copy_constructible<T>
  {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.front ().second;
  }

  [[nodiscard]] auto latest_time () const -> std::optional<time_point> {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.back ().first;
  }

  [[nodiscard]] auto oldest_time () const -> std::optional<time_point> {
    if (samples_.empty ()) {
      return std::nullopt;
    }
    return samples_.front ().first;
  }

  // True when the buffer spans at least window() from oldest to latest sample.
  [[nodiscard]] auto ready () const noexcept -> bool {
    if (samples_.empty ()) {
      return false;
    }
    return (samples_.back ().first - samples_.front ().first) >= window_;
  }

  [[nodiscard]] auto span () const -> duration {
    if (samples_.empty ()) {
      return duration::zero ();
    }
    return samples_.back ().first - samples_.front ().first;
  }

 private:
  template <typename U>
  auto push_impl (time_point t, U&& value) -> void {
    samples_.emplace_back (t, std::forward<U> (value));
    prune_older_than (t - window_);
  }

  auto prune_older_than (time_point cutoff) -> void {
    while (!samples_.empty () && samples_.front ().first < cutoff) {
      samples_.pop_front ();
    }
  }

  duration window_;
  std::deque<sample> samples_;
};

}  // namespace utils
