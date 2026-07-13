module;

#include <chrono>
#include <concepts>
#include <functional>
#include <utility>

export module utils.timing;

export namespace utils {

template <typename T>
concept ChronoDuration =
    requires {
      typename T::rep;
      typename T::period;
    } &&
    std::same_as<T, std::chrono::duration<typename T::rep, typename T::period>>;

template <ChronoDuration Duration = std::chrono::nanoseconds,
          typename F,
          typename... Args>
  requires std::invocable<F, Args...>
[[nodiscard]] Duration measure_time (F&& f, Args&&... args) {
  auto start = std::chrono::steady_clock::now ();
  std::invoke (std::forward<F> (f), std::forward<Args> (args)...);
  auto end = std::chrono::steady_clock::now ();

  return std::chrono::duration_cast<Duration> (end - start);
}
}  // namespace utils
