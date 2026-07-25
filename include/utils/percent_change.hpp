#pragma once

#include <cmath>
#include <concepts>
#include <optional>

namespace utils {

// Relative percent change from baseline to current:
//   (current - baseline) / baseline * 100
//
// Returns nullopt when baseline is zero or either argument is non-finite.
template <std::floating_point T>
[[nodiscard]] constexpr std::optional<T> percent_change (T baseline,
                                                         T current) noexcept {
  if (!std::isfinite (baseline) || !std::isfinite (current)) {
    return std::nullopt;
  }
  if (baseline == T {0}) {
    return std::nullopt;
  }
  return (current - baseline) / baseline * T {100};
}

}  // namespace utils
