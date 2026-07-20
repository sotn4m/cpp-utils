#include <utils/threadsafe_lookup_table.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Eq;
using testing::Optional;

namespace {

using StringIntTable = utils::threadsafe_lookup_table<std::string, int>;

TEST (ThreadSafeLookupTableTest, WrongSize) {
  using IntTable = utils::threadsafe_lookup_table<int, int>;
  EXPECT_THROW (IntTable (0), std::invalid_argument);
}

TEST (ThreadSafeLookupTableTest, MissingKeyReturnsNullopt) {
  StringIntTable table;
  EXPECT_THAT (table.value_for ("missing"), Eq (std::nullopt));
}

TEST (ThreadSafeLookupTableTest, InsertAndLookup) {
  StringIntTable table;
  table.add_or_update_mapping ("key", 42);

  EXPECT_THAT (table.value_for ("key"), Optional (Eq (42)));
}

TEST (ThreadSafeLookupTableTest, UpdateExistingKey) {
  StringIntTable table;
  table.add_or_update_mapping ("key", 1);
  table.add_or_update_mapping ("key", 2);

  EXPECT_THAT (table.value_for ("key"), Optional (Eq (2)));
}

TEST (ThreadSafeLookupTableTest, RemoveExistingKey) {
  StringIntTable table;
  table.add_or_update_mapping ("key", 1);
  table.remove_mapping ("key");

  EXPECT_THAT (table.value_for ("key"), Eq (std::nullopt));
}

TEST (ThreadSafeLookupTableTest, RemoveMissingKeyIsNoOp) {
  StringIntTable table;
  table.add_or_update_mapping ("key", 1);

  table.remove_mapping ("missing");

  EXPECT_THAT (table.value_for ("key"), Optional (Eq (1)));
}

TEST (ThreadSafeLookupTableTest, MoveInsertAvoidsCopy) {
  StringIntTable table;
  std::string key = "move-key";
  std::string key_copy = key;

  table.add_or_update_mapping (std::move (key), 7);

  EXPECT_TRUE (key.empty ());
  EXPECT_THAT (table.value_for (key_copy), Optional (Eq (7)));
}

TEST (ThreadSafeLookupTableTest, MoveUpdateExistingKey) {
  utils::threadsafe_lookup_table<std::string, std::string> table;
  table.add_or_update_mapping ("key", "old");

  std::string updated = "new";
  table.add_or_update_mapping (std::string {"key"}, std::move (updated));

  EXPECT_TRUE (updated.empty ());
  EXPECT_THAT (table.value_for ("key"), Optional (Eq ("new")));
}

TEST (ThreadSafeLookupTableTest, ConcurrentReadWhileWrite) {
  StringIntTable table;
  table.add_or_update_mapping ("key", 0);

  std::atomic<bool> writer_done {false};
  std::atomic<int> successful_reads {0};

  std::jthread writer ([&] {
    for (int i = 1; i <= 100; ++i) {
      table.add_or_update_mapping ("key", i);
    }
    writer_done.store (true, std::memory_order_release);
  });

  std::jthread reader ([&] {
    while (!writer_done.load (std::memory_order_acquire)) {
      if (auto value = table.value_for ("key")) {
        EXPECT_GE (*value, 0);
        ++successful_reads;
      }
    }
  });

  writer = {};
  reader = {};

  EXPECT_GT (successful_reads.load (), 0);
  EXPECT_THAT (table.value_for ("key"), Optional (Eq (100)));
}

TEST (ThreadSafeLookupTableTest, ConcurrentUpdatesSameKey) {
  StringIntTable table;
  constexpr int kThreads = 50;
  std::vector<std::jthread> threads;

  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back (
        [&table, i] { table.add_or_update_mapping ("key", i); });
  }
  threads.clear ();

  EXPECT_THAT (table.value_for ("key"), Optional (testing::Ge (0)));
  EXPECT_THAT (table.value_for ("key"), Optional (testing::Lt (kThreads)));
}

}  // namespace
