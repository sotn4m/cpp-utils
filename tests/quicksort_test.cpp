#include <gtest/gtest.h>
#include <deque>
#include <utils/quicksort.hpp>

namespace {
TEST (QuickSortTest, Empty) {
  std::vector<int> actual {};

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_TRUE (actual.empty ());
}

TEST (QuickSortTest, SingleElement) {
  std::vector actual {42};

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, std::vector {42});
}

TEST (QuickSortTest, AlreadySorted) {
  std::vector actual {1, 2, 3, 4, 5, 6, 7, 8};

  auto expected = actual;

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, ReverseSorted) {
  std::vector actual {8, 7, 6, 5, 4, 3, 2, 1};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, Duplicates) {
  std::vector actual {3, 1, 3, 2, 3, 1, 2, 3};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, NegativeNumbers) {
  std::vector actual {3, -1, 5, -10, 0, 2, -3};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, Array) {
  std::array actual {5, 2, 8, 1, 3};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, Deque) {
  std::deque actual {5, 2, 8, 1, 3};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, Double) {
  std::vector actual {3.14, 1.5, 2.7, -4.2, 0.0};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, String) {
  std::vector<std::string> actual {"pear", "apple", "orange", "banana"};

  auto expected = actual;
  std::ranges::sort (expected);

  utils::quicksort (actual, 0, actual.size ());

  EXPECT_EQ (actual, expected);
}

TEST (QuickSortTest, Subrange) {
  std::vector actual {99, 5, 2, 8, 1, 3, 77};

  utils::quicksort (actual, 1, 6);

  EXPECT_EQ (actual, std::vector ({99, 1, 2, 3, 5, 8, 77}));
}

}  // namespace
