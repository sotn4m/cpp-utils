#pragma once

#include <chrono>
#include <iostream>
#include <source_location>

namespace utils {

template <typename T>
concept ChronoDuration = requires {
  typename T::rep;
  typename T::period;
} && requires (T d) {
  std::chrono::duration<typename T::rep, typename T::period> {d};
};

template <ChronoDuration Duration = std::chrono::nanoseconds>
class scoped_timer {
 public:
  using ClockType = std::chrono::steady_clock;
  explicit scoped_timer (
      std::source_location loc = std::source_location::current ())
      : start_ {ClockType::now ()}, function_name_ {loc.function_name ()} {}

  scoped_timer (const scoped_timer&) = delete;
  scoped_timer (scoped_timer&&) = delete;

  auto operator= (const scoped_timer&) -> scoped_timer& = delete;
  auto operator= (scoped_timer&&) -> scoped_timer& = delete;

  ~scoped_timer () {
    using namespace std::chrono;
    auto duration =
        std::chrono::duration_cast<Duration> (ClockType::now () - start_);
    std::cout << function_name_ << " took " << duration << "\n";
  }

 private:
  const ClockType::time_point start_ {};
  std::string function_name_ {};
};

}  // namespace utils
