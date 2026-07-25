#include <utils/percent_change.hpp>

#include <limits>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using testing::DoubleNear;
using testing::Eq;
using testing::FloatEq;
using testing::Optional;

TEST (PercentChangeTest, ComputesPositiveMove) {
  EXPECT_THAT (utils::percent_change (1.08000, 1.08510),
               Optional (DoubleNear (0.472222, 1e-6)));
}

TEST (PercentChangeTest, ComputesNegativeMove) {
  EXPECT_THAT (utils::percent_change (100.0, 95.0), Optional (Eq (-5.0)));
}

TEST (PercentChangeTest, NoChangeReturnsZero) {
  EXPECT_THAT (utils::percent_change (1.2345, 1.2345), Optional (Eq (0.0)));
}

TEST (PercentChangeTest, ZeroBaselineReturnsNullopt) {
  EXPECT_THAT (utils::percent_change (0.0, 1.0), Eq (std::nullopt));
}

TEST (PercentChangeTest, NonFiniteReturnsNullopt) {
  double const inf = std::numeric_limits<double>::infinity ();
  double const nan = std::numeric_limits<double>::quiet_NaN ();

  EXPECT_THAT (utils::percent_change (nan, 1.0), Eq (std::nullopt));
  EXPECT_THAT (utils::percent_change (1.0, nan), Eq (std::nullopt));
  EXPECT_THAT (utils::percent_change (inf, 1.0), Eq (std::nullopt));
  EXPECT_THAT (utils::percent_change (1.0, inf), Eq (std::nullopt));
}

TEST (PercentChangeTest, WorksWithFloat) {
  EXPECT_THAT (utils::percent_change (2.0f, 3.0f), Optional (FloatEq (50.0f)));
}

}  // namespace
