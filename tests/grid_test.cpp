#include <utils/grid.hpp>

#include <array>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Eq;
using testing::Optional;

namespace {

TEST (GridTest, HeightAndWidth) {
  utils::grid<int> grid (3, 5);

  EXPECT_EQ (grid.height (), 3u);
  EXPECT_EQ (grid.width (), 5u);
}

TEST (GridTest, AtReturnsValueInBounds) {
  utils::grid<int> grid (2, 3);

  grid.fill_row (0, {1, 2, 3});
  grid.fill_row (1, {4, 5, 6});

  EXPECT_THAT (grid.at (0, 0), Optional (Eq (1)));
  EXPECT_THAT (grid.at (0, 2), Optional (Eq (3)));
  EXPECT_THAT (grid.at (1, 1), Optional (Eq (5)));
}

TEST (GridTest, AtReturnsNulloptWhenOutOfBounds) {
  utils::grid<int> grid (2, 3);

  EXPECT_THAT (grid.at (2, 0), Eq (std::nullopt));
  EXPECT_THAT (grid.at (0, 3), Eq (std::nullopt));
  EXPECT_THAT (grid.at (2, 3), Eq (std::nullopt));
}

TEST (GridTest, GridSuccess) {
  utils::grid<int> grid (2, 2);

  grid.fill_row (0, {1, 2});
  grid.fill_row (1, {3, 4});

  auto row0 = grid.get_row (0);
  auto row1 = grid.get_row (1);
  EXPECT_THAT (row0, ::testing::ElementsAreArray (std::array {1, 2}));
  EXPECT_THAT (row1, ::testing::ElementsAreArray (std::array {3, 4}));

  auto col0 = std::ranges::to<std::vector> (grid.get_col (0));
  auto col1 = std::ranges::to<std::vector> (grid.get_col (1));
  EXPECT_THAT (col0, ::testing::ElementsAre (1, 3));
  EXPECT_THAT (col1, ::testing::ElementsAre (2, 4));
}

TEST (GridTest, DefaultInitialized) {
  utils::grid<int> grid (2, 3);

  for (std::size_t row = 0; row < 2; ++row) {
    EXPECT_THAT (grid.get_row (row), ::testing::Each (::testing::Eq (0)));
  }
}

TEST (GridTest, FillRowRejectsTooManyValues) {
  utils::grid<int> grid (2, 2);

  EXPECT_FALSE (grid.fill_row (0, {1, 2, 3}));
  EXPECT_THAT (grid.get_row (0), ::testing::Each (::testing::Eq (0)));
}

TEST (GridTest, FillRowPartialLeavesRemainderUnchanged) {
  utils::grid<int> grid (2, 3);

  ASSERT_TRUE (grid.fill_row (0, {1, 2, 3}));
  ASSERT_TRUE (grid.fill_row (1, {4, 5, 6}));

  ASSERT_TRUE (grid.fill_row (0, {9}));
  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {9, 2, 3}));
  EXPECT_THAT (grid.get_row (1),
               ::testing::ElementsAreArray (std::array {4, 5, 6}));
}

TEST (GridTest, FillRowExactWidth) {
  utils::grid<int> grid (1, 4);

  EXPECT_TRUE (grid.fill_row (0, {1, 2, 3, 4}));
  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {1, 2, 3, 4}));
}

TEST (GridTest, LargerGridRowsAndCols) {
  utils::grid<int> grid (3, 3);

  grid.fill_row (0, {1, 2, 3});
  grid.fill_row (1, {4, 5, 6});
  grid.fill_row (2, {7, 8, 9});

  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {1, 2, 3}));
  EXPECT_THAT (grid.get_row (1),
               ::testing::ElementsAreArray (std::array {4, 5, 6}));
  EXPECT_THAT (grid.get_row (2),
               ::testing::ElementsAreArray (std::array {7, 8, 9}));

  EXPECT_THAT (std::ranges::to<std::vector> (grid.get_col (0)),
               ::testing::ElementsAre (1, 4, 7));
  EXPECT_THAT (std::ranges::to<std::vector> (grid.get_col (1)),
               ::testing::ElementsAre (2, 5, 8));
  EXPECT_THAT (std::ranges::to<std::vector> (grid.get_col (2)),
               ::testing::ElementsAre (3, 6, 9));
}

TEST (GridTest, RowViewIsMutable) {
  utils::grid<int> grid (2, 2);

  grid.fill_row (0, {1, 2});
  grid.fill_row (1, {3, 4});

  auto row0 = grid.get_row (0);
  row0[1] = 99;

  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {1, 99}));
  EXPECT_THAT (std::ranges::to<std::vector> (grid.get_col (1)),
               ::testing::ElementsAre (99, 4));
}

TEST (GridTest, ColViewIsMutable) {
  utils::grid<int> grid (2, 2);

  grid.fill_row (0, {1, 2});
  grid.fill_row (1, {3, 4});

  for (auto& value : grid.get_col (0)) {
    value += 100;
  }

  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {101, 2}));
  EXPECT_THAT (grid.get_row (1),
               ::testing::ElementsAreArray (std::array {103, 4}));
  EXPECT_THAT (std::ranges::to<std::vector> (grid.get_col (0)),
               ::testing::ElementsAre (101, 103));
}

TEST (GridTest, FillRowRejectsOutOfBoundsRow) {
  utils::grid<int> grid (2, 2);

  EXPECT_FALSE (grid.fill_row (2, {1, 2}));
  EXPECT_THAT (grid.get_row (0), ::testing::Each (::testing::Eq (0)));
}

TEST (GridTest, FillRowWithSpan) {
  utils::grid<int> grid (2, 3);
  const std::array values {1, 2, 3};

  EXPECT_TRUE (grid.fill_row (0, std::span {values}));
  EXPECT_THAT (grid.get_row (0), ::testing::ElementsAreArray (values));
}

TEST (GridTest, FillRowWithSpanPartial) {
  utils::grid<int> grid (2, 3);

  ASSERT_TRUE (grid.fill_row (0, {1, 2, 3}));
  ASSERT_TRUE (grid.fill_row (1, {4, 5, 6}));

  const std::array values {9};
  ASSERT_TRUE (grid.fill_row (0, std::span {values}));
  EXPECT_THAT (grid.get_row (0),
               ::testing::ElementsAreArray (std::array {9, 2, 3}));
}

TEST (GridTest, FillRowWithSpanRejectsTooManyValues) {
  utils::grid<int> grid (2, 2);
  const std::array values {1, 2, 3};

  EXPECT_FALSE (grid.fill_row (0, std::span {values}));
  EXPECT_THAT (grid.get_row (0), ::testing::Each (::testing::Eq (0)));
}

TEST (GridTest, FillRowWithSpanRejectsOutOfBoundsRow) {
  utils::grid<int> grid (2, 2);
  const std::array values {1, 2};

  EXPECT_FALSE (grid.fill_row (2, std::span {values}));
}

TEST (GridTest, FillRowWithSingleValue) {
  utils::grid<int> grid (2, 3);

  EXPECT_TRUE (grid.fill_row (0, 7));
  EXPECT_THAT (grid.get_row (0), ::testing::Each (::testing::Eq (7)));
}

TEST (GridTest, FillRowWithSingleValueRejectsOutOfBoundsRow) {
  utils::grid<int> grid (2, 3);

  EXPECT_FALSE (grid.fill_row (2, 7));
}

TEST (GridTest, ConstGetRowAndGetCol) {
  utils::grid<int> grid (2, 3);
  grid.fill_row (0, {1, 2, 3});
  grid.fill_row (1, {4, 5, 6});

  const auto& cgrid = grid;
  EXPECT_THAT (cgrid.get_row (0),
               ::testing::ElementsAreArray (std::array {1, 2, 3}));
  EXPECT_THAT (cgrid.get_row (1),
               ::testing::ElementsAreArray (std::array {4, 5, 6}));
  EXPECT_THAT (std::ranges::to<std::vector> (cgrid.get_col (0)),
               ::testing::ElementsAre (1, 4));
  EXPECT_THAT (std::ranges::to<std::vector> (cgrid.get_col (2)),
               ::testing::ElementsAre (3, 6));
}

TEST (GridTest, OperatorCallMutable) {
  utils::grid<int> grid (2, 2);

  grid (0, 0) = 1;
  grid (0, 1) = 2;
  grid (1, 0) = 3;
  grid (1, 1) = 4;

  EXPECT_EQ (grid (0, 0), 1);
  EXPECT_EQ (grid (0, 1), 2);
  EXPECT_EQ (grid (1, 0), 3);
  EXPECT_EQ (grid (1, 1), 4);

  grid (0, 1) = 99;
  EXPECT_EQ (grid (0, 1), 99);
}

TEST (GridTest, OperatorCallConst) {
  utils::grid<int> grid (2, 2);
  grid.fill_row (0, {1, 2});
  grid.fill_row (1, {3, 4});

  const auto& cgrid = grid;
  EXPECT_EQ (cgrid (0, 0), 1);
  EXPECT_EQ (cgrid (0, 1), 2);
  EXPECT_EQ (cgrid (1, 0), 3);
  EXPECT_EQ (cgrid (1, 1), 4);
}

}  // namespace
