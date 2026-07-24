#pragma once
#include <initializer_list>
#include <optional>
#include <ranges>
#include <vector>

namespace utils {

template <typename T>
class grid {
 public:
  grid (std::size_t h, std::size_t w) : h_ {h}, w_ {w} {
    data_.resize (h_ * w_);
  }

  auto get_row (std::size_t n);
  auto get_col (std::size_t n);
  auto at (std::size_t row, std::size_t col) -> std::optional<T>;
  auto height () const { return h_; }
  auto width () const { return w_; }
  bool fill_row (std::size_t n, std::initializer_list<T> values);

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
auto grid<T>::at (std::size_t row, std::size_t col) -> std::optional<T> {
  if (row >= h_ || col >= w_) {
    return std::nullopt;
  }

  return data_[row * w_ + col];
}

template <typename T>
auto grid<T>::get_col (std::size_t n) {
  return std::views::iota (std::size_t {0}, h_) |
         std::views::transform (
             [this, n] (std::size_t row) -> T& { return data_[row * w_ + n]; });
}

template <typename T>
bool grid<T>::fill_row (std::size_t n, std::initializer_list<T> values) {
  auto size = std::ranges::distance (values);

  if (size > w_) {
    return false;
  }

  auto index = data_.begin () + n * w_;
  std::ranges::copy (values, index);
  return true;
}

}  // namespace utils
