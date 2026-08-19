#include <utils/lru_cache.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

using testing::Eq;
using testing::Optional;

namespace {

TEST (LruCacheTest, EmptyCache) {
  utils::lru_cache<int, int> cache {10};

  auto x = cache.get (5);
  EXPECT_THAT (x, Eq (std::nullopt));
}

TEST (LruCacheTest, PutThenGet) {
  utils::lru_cache<int, int> cache {10};

  cache.put (1, 100);

  EXPECT_THAT (cache.get (1), Optional (Eq (100)));
}

TEST (LruCacheTest, MultiplePutsWithinCapacity) {
  utils::lru_cache<int, int> cache {3};

  cache.put (1, 10);
  cache.put (2, 20);
  cache.put (3, 30);

  EXPECT_THAT (cache.get (1), Optional (Eq (10)));
  EXPECT_THAT (cache.get (2), Optional (Eq (20)));
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

TEST (LruCacheTest, EvictsLeastRecentlyUsedOnOverflow) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);
  cache.put (3, 30);  // evicts key 1, the least recently touched

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.get (2), Optional (Eq (20)));
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

TEST (LruCacheTest, GetRefreshesRecency) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);
  cache.get (1);      // touch key 1, key 2 becomes least recently used
  cache.put (3, 30);  // evicts key 2, not key 1

  EXPECT_THAT (cache.get (1), Optional (Eq (10)));
  EXPECT_THAT (cache.get (2), Eq (std::nullopt));
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

TEST (LruCacheTest, PutExistingKeyUpdatesValueAndRecency) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);
  cache.put (1, 999);  // update key 1, also makes it most recently used
  cache.put (3, 30);   // evicts key 2, not key 1

  EXPECT_THAT (cache.get (1), Optional (Eq (999)));
  EXPECT_THAT (cache.get (2), Eq (std::nullopt));
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

TEST (LruCacheTest, PutExistingKeyDoesNotGrowSize) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (1, 20);  // update, should not consume an extra capacity slot
  cache.put (2, 30);

  EXPECT_THAT (cache.get (1), Optional (Eq (20)));
  EXPECT_THAT (cache.get (2), Optional (Eq (30)));
}

TEST (LruCacheTest, CapacityOneAlwaysEvictsPrevious) {
  utils::lru_cache<int, int> cache {1};

  cache.put (1, 10);
  cache.put (2, 20);

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.get (2), Optional (Eq (20)));
}

TEST (LruCacheTest, InterleavedGetPutEvictionOrder) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 1);
  cache.put (2, 2);
  EXPECT_THAT (cache.get (1), Optional (Eq (1)));  // touches 1; order: 1, 2

  cache.put (3, 3);  // evicts 2; order: 3, 1
  EXPECT_THAT (cache.get (2), Eq (std::nullopt));
  EXPECT_THAT (cache.get (3), Optional (Eq (3)));

  cache.put (4, 4);  // evicts 1; order: 4, 3
  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.get (3), Optional (Eq (3)));
  EXPECT_THAT (cache.get (4), Optional (Eq (4)));
}

TEST (LruCacheTest, GetOnEvictedKeyReturnsNullopt) {
  utils::lru_cache<int, int> cache {1};

  cache.put (1, 10);
  cache.put (2, 20);

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
}

TEST (LruCacheTest, ZeroCapacityNeverStoresAnything) {
  utils::lru_cache<int, int> cache {0};

  cache.put (1, 10);

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
}

TEST (LruCacheTest, StringKeyAndValue) {
  utils::lru_cache<std::string, std::string> cache {2};

  cache.put ("a", "apple");
  cache.put ("b", "banana");
  cache.put ("c", "cherry");  // evicts "a"

  EXPECT_THAT (cache.get ("a"), Eq (std::nullopt));
  EXPECT_THAT (cache.get ("b"), Optional (Eq ("banana")));
  EXPECT_THAT (cache.get ("c"), Optional (Eq ("cherry")));
}

TEST (LruCacheTest, GetReturnsIndependentCopy) {
  utils::lru_cache<int, int> cache {10};

  cache.put (1, 100);

  auto result = cache.get (1);
  ASSERT_TRUE (result.has_value ());
  *result = 999;  // mutating the returned copy must not affect the cache

  EXPECT_THAT (cache.get (1), Optional (Eq (100)));
}

TEST (LruCacheTest, SizeReflectsEntryCount) {
  utils::lru_cache<int, int> cache {3};

  EXPECT_THAT (cache.size (), Eq (0u));

  cache.put (1, 10);
  cache.put (2, 20);
  EXPECT_THAT (cache.size (), Eq (2u));

  cache.put (1, 999);  // update, not a new entry
  EXPECT_THAT (cache.size (), Eq (2u));

  cache.put (3, 30);
  cache.put (4, 40);  // evicts one entry, capacity is 3
  EXPECT_THAT (cache.size (), Eq (3u));
}

TEST (LruCacheTest, ContainsDoesNotAffectRecency) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);

  EXPECT_TRUE (cache.contains (1));
  EXPECT_TRUE (cache.contains (2));
  EXPECT_FALSE (cache.contains (3));

  cache.put (3, 30);  // contains() should not have protected key 1 from eviction

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.get (2), Optional (Eq (20)));
}

TEST (LruCacheTest, EraseRemovesEntry) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);

  EXPECT_TRUE (cache.erase (1));
  EXPECT_FALSE (cache.erase (1));  // already gone

  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.size (), Eq (1u));

  cache.put (3, 30);  // freed slot should be usable again
  EXPECT_THAT (cache.get (2), Optional (Eq (20)));
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

TEST (LruCacheTest, ClearRemovesAllEntries) {
  utils::lru_cache<int, int> cache {2};

  cache.put (1, 10);
  cache.put (2, 20);
  cache.clear ();

  EXPECT_THAT (cache.size (), Eq (0u));
  EXPECT_THAT (cache.get (1), Eq (std::nullopt));
  EXPECT_THAT (cache.get (2), Eq (std::nullopt));

  cache.put (3, 30);  // cache should be fully usable after clear
  EXPECT_THAT (cache.get (3), Optional (Eq (30)));
}

}  // namespace
