#pragma once
#include <algorithm>

namespace utils {

template <typename T>
size_t partition_ (T& cntr, std::size_t p, std::size_t r) {
  size_t last_elem_index = r - 1;
  auto x = cntr[last_elem_index];
  auto i = p;
  for (auto j = i; j < last_elem_index; ++j) {
    if (cntr[j] <= x) {
      std::swap (cntr[i], cntr[j]);
      ++i;
    }
  }
  std::swap (cntr[i], cntr[last_elem_index]);
  return i;
}

template <typename T>
void quicksort (T& cntr, std::size_t p, std::size_t r) {
  if (p < r) {
    auto q = partition_ (cntr, p, r);
    quicksort (cntr, p, q);
    quicksort (cntr, q + 1, r);
  }
}

}  // namespace utils
