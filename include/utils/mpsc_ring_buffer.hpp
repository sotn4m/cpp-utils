#pragma once
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>

template <std::size_t N>
concept ValidSize = N > 2 && std::has_single_bit (N);

namespace utils {

inline constexpr std::size_t cache_line_size {64};

template <typename T, std::size_t N>
  requires ValidSize<N>
class mpsc_ring_buffer {
 public:
  explicit mpsc_ring_buffer ();
  [[nodiscard]] bool try_push (const T& item);
  [[nodiscard]] bool try_push (T&& item);
  [[nodiscard]] std::optional<T> try_pop ();

  [[nodiscard]] std::size_t capacity () const noexcept { return N; }
  [[nodiscard]] std::size_t size () const noexcept;
  [[nodiscard]] bool empty () const noexcept;
  [[nodiscard]] bool full () const noexcept;

 private:
  struct Slot {
    T val_;
    std::atomic<uint64_t> sequence_;
  };

  std::array<Slot, N> buffer_ {};
  alignas (cache_line_size) std::atomic<uint64_t> write_ {0};
  alignas (cache_line_size) std::atomic<uint64_t> read_ {0};
};

template <typename T, std::size_t N>
  requires ValidSize<N>
mpsc_ring_buffer<T, N>::mpsc_ring_buffer () {
  for (std::size_t i {0}; i < N; ++i) {
    buffer_[i].sequence_ = i;
  }
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool mpsc_ring_buffer<T, N>::try_push (const T& item) {
  auto write = write_.load (std::memory_order_acquire);
  while (true) {
    auto read = read_.load (std::memory_order_acquire);
    if ((write - read) == N) {
      return false;
    }
    auto write_index = write & (capacity () - 1);
    auto sequence =
        buffer_[write_index].sequence_.load (std::memory_order_acquire);
    if (sequence != write) {
      // we cannot write the slot if it was not cleared
      return false;
    }

    auto desired = write + 1;
    if (write_.compare_exchange_weak (write, desired, std::memory_order_release,
                                      std::memory_order_acquire)) {
      buffer_[write_index].val_ = item;
      buffer_[write_index].sequence_.store (desired, std::memory_order_release);
      break;
    }
  }

  return true;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool mpsc_ring_buffer<T, N>::try_push (T&& item) {
  auto write = write_.load (std::memory_order_acquire);
  while (true) {
    auto read = read_.load (std::memory_order_acquire);
    if ((write - read) == N) {
      return false;
    }
    auto write_index = write & (capacity () - 1);
    auto sequence =
        buffer_[write_index].sequence_.load (std::memory_order_acquire);
    if (sequence != write) {
      return false;
    }

    auto desired = write + 1;
    if (write_.compare_exchange_weak (write, desired, std::memory_order_release,
                                      std::memory_order_acquire)) {
      buffer_[write_index].val_ = std::move (item);
      buffer_[write_index].sequence_.store (desired, std::memory_order_release);
      break;
    }
  }

  return true;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
std::optional<T> mpsc_ring_buffer<T, N>::try_pop () {
  auto read = read_.load (std::memory_order_relaxed);

  auto read_index = read & (capacity () - 1);
  auto sequence =
      buffer_[read_index].sequence_.load (std::memory_order_acquire);

  if (sequence != (read + 1)) {
    return std::nullopt;
  }

  T elem = std::move (buffer_[read_index].val_);
  buffer_[read_index].sequence_.store (read + N, std::memory_order_release);
  read_.store (++read, std::memory_order_release);
  return elem;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
std::size_t mpsc_ring_buffer<T, N>::size () const noexcept {
  auto write = write_.load (std::memory_order_acquire);
  auto read = read_.load (std::memory_order_acquire);
  return write - read;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool mpsc_ring_buffer<T, N>::full () const noexcept {
  // Full is called by the producer
  auto write = write_.load (std::memory_order_acquire);
  auto read = read_.load (std::memory_order_acquire);
  return (write - read) == N;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool mpsc_ring_buffer<T, N>::empty () const noexcept {
  auto read = read_.load (std::memory_order_acquire);
  auto write = write_.load (std::memory_order_acquire);
  return write == read;
}

}  // namespace utils
