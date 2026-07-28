#include <utils/cooldown_tracker.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Eq;
using testing::Optional;

namespace {

struct manual_clock {
  using rep = std::int64_t;
  using period = std::milli;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<manual_clock, duration>;

  static constexpr bool is_steady = true;

  static time_point now () noexcept {
    return time_point {duration {current_ms_}};
  }

  static void set (time_point tp) noexcept {
    current_ms_ = tp.time_since_epoch ().count ();
  }

  static void advance (duration d) noexcept { current_ms_ += d.count (); }

  static void reset () noexcept { current_ms_ = 0; }

 private:
  static inline rep current_ms_ {0};
};

using minutes = std::chrono::minutes;
using trigger_time = manual_clock::time_point;

TEST (CooldownTrackerTest, FirstTriggerSucceeds) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
}

TEST (CooldownTrackerTest, ImmediateRetriggerBlocked) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Eq (std::nullopt));
}

TEST (CooldownTrackerTest, RetriggerAllowedAfterCooldown) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  manual_clock::advance (minutes {15});
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {15}})));
}

TEST (CooldownTrackerTest, RetriggerBlockedJustBeforeCooldownExpires) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  manual_clock::advance (minutes {15} - std::chrono::milliseconds {1});
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Eq (std::nullopt));
}

TEST (CooldownTrackerTest, IndependentKeys) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger ("GBP_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Eq (std::nullopt));
}

TEST (CooldownTrackerTest, LastTriggerReturnsNulloptBeforeFirstTrigger) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  EXPECT_THAT (tracker.last_trigger ("EUR_USD:move_5m"), Eq (std::nullopt));
}

TEST (CooldownTrackerTest, LastTriggerReturnsTimeAfterSuccessfulTrigger) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  manual_clock::set (trigger_time {minutes {42}});
  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {42}})));

  EXPECT_THAT (tracker.last_trigger ("EUR_USD:move_5m"),
               Optional (Eq (trigger_time {minutes {42}})));
}

TEST (CooldownTrackerTest, LastTriggerUnchangedWhenTryTriggerReturnsNullopt) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  manual_clock::set (trigger_time {minutes {10}});
  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {10}})));

  manual_clock::set (trigger_time {minutes {11}});
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Eq (std::nullopt));

  EXPECT_THAT (tracker.last_trigger ("EUR_USD:move_5m"),
               Optional (Eq (trigger_time {minutes {10}})));
}

TEST (CooldownTrackerTest, ResetClearsCooldown) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Eq (std::nullopt));

  tracker.reset ("EUR_USD:move_5m");
  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
}

TEST (CooldownTrackerTest, ClearClearsAllCooldowns) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  ASSERT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  ASSERT_THAT (tracker.try_trigger ("GBP_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));

  tracker.clear ();

  EXPECT_THAT (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger ("GBP_USD:move_5m", minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
}

TEST (CooldownTrackerTest, RejectsNonPositiveCooldown) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  EXPECT_THROW (
      static_cast<void> (tracker.try_trigger ("EUR_USD:move_5m", minutes {0})),
      std::invalid_argument);
  EXPECT_THROW (
      static_cast<void> (tracker.try_trigger ("EUR_USD:move_5m", -minutes {1})),
      std::invalid_argument);
}

TEST (CooldownTrackerTest, ConcurrentTriggersAllowAtMostOnePerWindow) {
  manual_clock::reset ();
  utils::cooldown_tracker<std::string, manual_clock> tracker;

  constexpr int kThreads = 8;
  std::atomic<int> success_count {0};
  std::vector<std::thread> threads;
  threads.reserve (kThreads);

  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back ([&] {
      if (tracker.try_trigger ("EUR_USD:move_5m", minutes {15}).has_value ()) {
        success_count.fetch_add (1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : threads) {
    t.join ();
  }

  EXPECT_EQ (success_count.load (), 1);
}

struct rule_key {
  std::string instrument;
  std::string rule;

  bool operator== (rule_key const& other) const = default;
};

}  // namespace

template <>
struct std::hash<rule_key> {
  std::size_t operator() (rule_key const& key) const noexcept {
    return std::hash<std::string> {}(key.instrument + '\0' + key.rule);
  }
};

namespace {

TEST (CooldownTrackerTest, CustomKeyType) {
  manual_clock::reset ();
  utils::cooldown_tracker<rule_key, manual_clock> tracker;

  rule_key const key {.instrument = "EUR_USD", .rule = "move_5m"};

  EXPECT_THAT (tracker.try_trigger (key, minutes {15}),
               Optional (Eq (trigger_time {minutes {0}})));
  EXPECT_THAT (tracker.try_trigger (key, minutes {15}), Eq (std::nullopt));
}

}  // namespace
