#pragma once
#include <cassert>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

namespace utils {

template <typename T>
class grid {
 public:
  grid (std::size_t h, std::size_t w) : h_ {h}, w_ {w} {
    data_.resize (h_ * w_);
  }

  auto get_row (std::size_t n);
  auto get_row (std::size_t n) const;
  auto get_col (std::size_t n);
  auto get_col (std::size_t n) const;
  auto at (std::size_t row, std::size_t col) -> std::optional<T>;

  auto operator() (std::size_t row, std::size_t col) -> T&;
  const T& operator() (std::size_t row, std::size_t col) const;

  auto height () const { return h_; }
  auto width () const { return w_; }
  auto fill_row (std::size_t n, std::initializer_list<T> values) -> bool;
  auto fill_row (std::size_t n, std::span<const T> values) -> bool;
  auto fill_row (std::size_t n, const T& value) -> bool;

 private:
  std::vector<T> data_ {};
  std::size_t h_ {};
  std::size_t w_ {};
};

//
//   1 2 3
// h 4 5 6   -> 1 2 3 4 5 6 7 8 9
//   7 8 9
//     w
//
template <typename T>
auto grid<T>::get_row (std::size_t n) {
  auto start = data_.begin () + n * w_;
  return std::views::counted (start, w_);
}

template <typename T>
auto grid<T>::get_row (std::size_t n) const {
  auto start = data_.cbegin () + n * w_;
  return std::views::counted (start, w_);
}

template <typename T>
auto grid<T>::at (std::size_t row, std::size_t col) -> std::optional<T> {
  if (row >= h_ || col >= w_) {
    return std::nullopt;
  }

  return data_[row * w_ + col];
}

template <typename T>
auto grid<T>::operator() (std::size_t row, std::size_t col) const -> const T& {
  assert (row < h_ && col < w_);
  return data_[row * w_ + col];
}

template <typename T>
auto grid<T>::operator() (std::size_t row, std::size_t col) -> T& {
  return data_[row * w_ + col];
}

template <typename T>
auto grid<T>::get_col (std::size_t n) {
  return std::views::iota (std::size_t {0}, h_) |
         std::views::transform (
             [this, n] (std::size_t row) -> T& { return data_[row * w_ + n]; });
}

template <typename T>
auto grid<T>::get_col (std::size_t n) const {
  return std::views::iota (std::size_t {0}, h_) |
         std::views::transform ([this, n] (std::size_t row) -> const T& {
           return data_[row * w_ + n];
         });
}

template <typename T>
bool grid<T>::fill_row (std::size_t n, std::initializer_list<T> values) {
  auto size = std::ranges::distance (values);

  if (size > w_ || n >= h_) {
    return false;
  }

  auto index = data_.begin () + n * w_;
  std::ranges::copy (values, index);
  return true;
}

template <typename T>
bool grid<T>::fill_row (std::size_t n, std::span<const T> values) {
  if (values.size () > w_ || n >= h_) {
    return false;
  }

  auto first = data_.begin () + n * w_;
  std::ranges::copy (values, first);
  return true;
}

template <typename T>
bool grid<T>::fill_row (std::size_t n, const T& value) {
  if (n >= h_) {
    return false;
  }

  auto first = data_.begin () + n * w_;
  std::ranges::fill_n (first, w_, value);
  return true;
}

}  // namespace utils
