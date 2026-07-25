#include <optional>
#include <utils/rolling_time_window.hpp>

#include <chrono>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using clock = std::chrono::steady_clock;
using time_point = clock::time_point;
using minutes = std::chrono::minutes;

using testing::DoubleEq;
using testing::DoubleNear;
using testing::Eq;
using testing::Optional;

time_point tp (int minutes_from_epoch) {
  return time_point {} + minutes {minutes_from_epoch};
}

TEST (RollingTimeWindowTest, EmptyOnConstruction) {
  utils::rolling_time_window<double, clock> window {minutes {5}};

  EXPECT_TRUE (window.empty ());
  EXPECT_EQ (window.size (), 0u);
  EXPECT_FALSE (window.ready ());
  EXPECT_EQ (window.span (), clock::duration::zero ());
  EXPECT_THAT (window.latest_value (), Eq (std::nullopt));
  EXPECT_THAT (window.oldest_value (), Eq (std::nullopt));
}

TEST (RollingTimeWindowTest, RejectsZeroWindow) {
  EXPECT_THROW (
      (utils::rolling_time_window<int, clock> {clock::duration::zero ()}),
      std::invalid_argument);
  EXPECT_THROW ((utils::rolling_time_window<int, clock> {-minutes {1}}),
                std::invalid_argument);
}

TEST (RollingTimeWindowTest, SinglePushNotReady) {
  utils::rolling_time_window<double, clock> window {minutes {5}};

  window.push (tp (1), 100.0);

  EXPECT_EQ (window.size (), 1u);
  EXPECT_FALSE (window.ready ());
  EXPECT_THAT (window.latest_value (), Optional (DoubleEq (100.0)));
  EXPECT_THAT (window.oldest_value (), Optional (DoubleEq (100.0)));
}

TEST (RollingTimeWindowTest, ReadyAfterFullSpan) {
  utils::rolling_time_window<double, clock> window {minutes {5}};

  window.push (tp (0), 100.0);
  window.push (tp (5), 105.0);

  EXPECT_TRUE (window.ready ());
  EXPECT_EQ (window.span (), minutes {5});
  EXPECT_THAT (window.oldest_value (), Optional (DoubleEq (100.0)));
  EXPECT_THAT (window.latest_value (), Optional (DoubleEq (105.0)));
}

TEST (RollingTimeWindowTest, PrunesSamplesOutsideWindow) {
  utils::rolling_time_window<int, clock> window {minutes {5}};

  window.push (tp (0), 1);
  window.push (tp (1), 2);
  window.push (tp (3), 3);
  window.push (tp (7), 4);

  ASSERT_EQ (window.size (), 2u);
  EXPECT_THAT (window.oldest_time (), Optional (Eq (tp (3))));
  EXPECT_THAT (window.latest_time (), Optional (Eq (tp (7))));
  EXPECT_THAT (window.oldest_value (), Optional (Eq (3)));
  EXPECT_THAT (window.latest_value (), Optional (Eq (4)));
}

TEST (RollingTimeWindowTest, ClearResetsState) {
  utils::rolling_time_window<int, clock> window {minutes {5}};

  window.push (tp (0), 1);
  window.push (tp (10), 2);
  window.clear ();

  EXPECT_TRUE (window.empty ());
  EXPECT_FALSE (window.ready ());
}

TEST (RollingTimeWindowTest, MoveOnlyValueType) {
  utils::rolling_time_window<std::unique_ptr<int>, clock> window {minutes {5}};

  window.push (tp (0), std::make_unique<int> (7));
  window.push (tp (5), std::make_unique<int> (9));

  EXPECT_TRUE (window.ready ());
  EXPECT_EQ (window.size (), 2u);
  EXPECT_THAT (window.oldest_time (), Optional (Eq (tp (0))));
  EXPECT_THAT (window.latest_time (), Optional (Eq (tp (5))));
}

TEST (RollingTimeWindowTest, ComputesFiveMinuteMoveScenario) {
  utils::rolling_time_window<double, clock> window {minutes {5}};

  window.push (tp (0), 1.08000);
  window.push (tp (1), 1.08100);
  window.push (tp (2), 1.08200);
  window.push (tp (3), 1.08300);
  window.push (tp (4), 1.08400);
  window.push (tp (5), 1.08510);

  ASSERT_TRUE (window.ready ());
  EXPECT_THAT (window.oldest_value (), Optional (DoubleEq (1.08000)));
  EXPECT_THAT (window.latest_value (), Optional (DoubleEq (1.08510)));

  double const oldest = *window.oldest_value ();
  double const latest = *window.latest_value ();
  double const change_pct = (latest - oldest) / oldest * 100.0;
  EXPECT_THAT (change_pct, DoubleNear (0.472222, 1e-6));
}

}  // namespace
