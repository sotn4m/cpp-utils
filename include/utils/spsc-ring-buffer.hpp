#pragma once
#include <array>
#include <atomic>
#include <bit>
#include <optional>

template <std::size_t N>
concept ValidSize = N > 2 && std::has_single_bit (N);

namespace utils {

template <typename T, std::size_t N>
  requires ValidSize<N>
class spsc_ring_buffer {
 public:
  explicit spsc_ring_buffer () = default;
  [[nodiscard]] bool try_push (const T& item);
  [[nodiscard]] bool try_push (T&& item);
  [[nodiscard]] std::optional<T> try_pop ();

  [[nodiscard]] std::size_t capacity () const noexcept { return N; }
  [[nodiscard]] std::size_t size () const noexcept;
  [[nodiscard]] bool empty () const noexcept;
  [[nodiscard]] bool full () const noexcept;

 private:
  std::array<T, N> buffer_;
  std::atomic<uint64_t> write_ {0};
  std::atomic<uint64_t> read_ {0};
};

template <typename T, std::size_t N>
  requires ValidSize<N>
bool spsc_ring_buffer<T, N>::try_push (const T& item) {
  // can we use the full method?
  // if (this->full()) return false;
  auto write = write_.load (std::memory_order_relaxed);
  auto read = read_.load (std::memory_order_acquire);

  if ((write - read) == N)
    return false;

  auto write_index = write & (capacity () - 1);
  buffer_[write_index] = item;
  write_.store (++write, std::memory_order_release);
  return true;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool spsc_ring_buffer<T, N>::try_push (T&& item) {
  auto write = write_.load (std::memory_order_relaxed);
  auto read = read_.load (std::memory_order_acquire);

  if ((write - read) == N)
    return false;

  auto write_index = write & (capacity () - 1);
  buffer_[write_index] = std::move (item);
  write_.store (++write, std::memory_order_release);
  return true;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
std::optional<T> spsc_ring_buffer<T, N>::try_pop () {
  auto read = read_.load (std::memory_order_relaxed);
  auto write = write_.load (std::memory_order_acquire);

  if (read == write)
    return std::nullopt;

  auto read_index = read & (capacity () - 1);
  auto elem = buffer_[read_index];
  read_.store (++read, std::memory_order_release);
  return elem;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
std::size_t spsc_ring_buffer<T, N>::size () const noexcept {
  auto write = write_.load (std::memory_order_acquire);
  auto read = read_.load (std::memory_order_acquire);
  return write - read;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool spsc_ring_buffer<T, N>::full () const noexcept {
  // Full is called by the producer
  auto write = write_.load (std::memory_order_relaxed);
  auto read = read_.load (std::memory_order_acquire);
  return (write - read) == N;
}

template <typename T, std::size_t N>
  requires ValidSize<N>
bool spsc_ring_buffer<T, N>::empty () const noexcept {
  // Called by the consumer; load on read_ can mem_ordr_relaxed
  auto read = read_.load (std::memory_order_relaxed);
  // Producer modifies write_; we need mem_ordr_acquire
  auto write = write_.load (std::memory_order_acquire);
  return write == read;
}

}  // namespace utils
