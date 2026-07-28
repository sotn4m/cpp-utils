#pragma once

#include <chrono>
#include <concepts>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace utils {

// Fixed cooldown gate: at most one successful trigger per key per cooldown
// interval.
//
// try_trigger(key, cooldown) returns the trigger time when the key has never
// fired or when at least cooldown has elapsed since the last successful
// trigger; in that case the trigger time is updated. Returns nullopt when still
// in cooldown without updating.
//
// last_trigger(key) returns the stored trigger time without attempting a new
// trigger, or nullopt if the key has never successfully triggered.
// Thread-safe: all public methods take the same internal lock. Intended for low
// cardinality keys (e.g. instrument + rule id), not high-frequency per-tick
// use.
//
// Clock defaults to std::chrono::steady_clock so cooldown math is unaffected by
// wall-clock adjustments.
template <typename Key, typename Clock = std::chrono::steady_clock>
  requires requires {
    typename Clock::time_point;
    typename Clock::duration;
    { Clock::now () } -> std::same_as<typename Clock::time_point>;
  }

class cooldown_tracker {
 public:
  using clock = Clock;
  using time_point = typename clock::time_point;
  using duration = typename clock::duration;

  cooldown_tracker () = default;

  cooldown_tracker (const cooldown_tracker&) = delete;
  cooldown_tracker& operator= (const cooldown_tracker&) = delete;
  cooldown_tracker (cooldown_tracker&&) = delete;
  cooldown_tracker& operator= (cooldown_tracker&&) = delete;

  [[nodiscard]] auto try_trigger (const Key& key, duration cooldown)
      -> std::optional<time_point> {
    if (cooldown <= duration::zero ()) {
      throw std::invalid_argument ("cooldown duration must be positive");
    }
    const auto now = clock::now ();
    std::lock_guard lock {mutex_};

    const auto it = last_trigger_.find (key);
    if (it == last_trigger_.end () || (now - it->second) >= cooldown) {
      last_trigger_.insert_or_assign (key, now);
      return now;
    }
    return std::nullopt;
  }

  [[nodiscard]] auto last_trigger (Key const& key) const
      -> std::optional<time_point> {
    std::lock_guard lock {mutex_};
    if (const auto it = last_trigger_.find (key); it != last_trigger_.end ()) {
      return it->second;
    }
    return std::nullopt;
  }

  auto reset (const Key& key) -> void {
    std::lock_guard lock {mutex_};
    last_trigger_.erase (key);
  }

  auto clear () noexcept -> void {
    std::lock_guard lock {mutex_};
    last_trigger_.clear ();
  }

 private:
  mutable std::mutex mutex_;
  std::unordered_map<Key, time_point> last_trigger_;
};

}  // namespace utils
