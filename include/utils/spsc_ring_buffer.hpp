#pragma once
#include <array>
#include <atomic>
#include <bit>
#include <optional>

template <std::size_t N>
concept ValidSize = N > 2 && std::has_single_bit (N);

namespace utils {

inline constexpr std::size_t cache_line_size {64};

// Lock-free single-producer, single-consumer ring buffer.
//
// Threading: exactly one thread may call try_push, size(), and full(); exactly
// one other thread may call try_pop, size(), and empty(). Any other use pattern
// is undefined behavior.
//
// Capacity is N, a power of two greater than 2. All N slots may hold data; no
// slot is reserved to distinguish full from empty.
//
// try_push returns false when full; try_pop returns nullopt when empty. Neither
// call blocks.
//
// size(), empty(), and full() are snapshots and may be briefly stale while the
// other thread is active. Prefer calling full() from the producer and empty()
// from the consumer.
//
// T must be default-constructible (backing storage is a fixed std::array). Copy
// and move overloads of try_push are provided; T must support the operation
// used.
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
  alignas (cache_line_size) std::atomic<uint64_t> write_ {0};
  alignas (cache_line_size) std::atomic<uint64_t> read_ {0};
};

template <typename T, std::size_t N>
  requires ValidSize<N>
bool spsc_ring_buffer<T, N>::try_push (const T& item) {
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
  T elem = std::move (buffer_[read_index]);
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
