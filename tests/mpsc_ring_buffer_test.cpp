#include <utils/mpsc_ring_buffer.hpp>

#include <atomic>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <gmock/gmock.h>

using testing::Eq;

namespace {

TEST (MpscRingBufferTest, EmptyOnConstruction) {
  utils::mpsc_ring_buffer<int, 8> buf;

  EXPECT_TRUE (buf.empty ());
  EXPECT_FALSE (buf.full ());
  EXPECT_EQ (buf.size (), 0u);
  EXPECT_EQ (buf.capacity (), 8u);
}

TEST (MpscRingBufferTest, SingleProducerPushPopFifo) {
  utils::mpsc_ring_buffer<int, 8> buf;

  ASSERT_TRUE (buf.try_push (10));
  ASSERT_TRUE (buf.try_push (20));
  ASSERT_TRUE (buf.try_push (30));

  EXPECT_EQ (buf.size (), 3u);

  auto first = buf.try_pop ();
  auto second = buf.try_pop ();
  auto third = buf.try_pop ();

  ASSERT_TRUE (first.has_value ());
  ASSERT_TRUE (second.has_value ());
  ASSERT_TRUE (third.has_value ());
  EXPECT_EQ (*first, 10);
  EXPECT_EQ (*second, 20);
  EXPECT_EQ (*third, 30);
  EXPECT_TRUE (buf.empty ());
}

TEST (MpscRingBufferTest, FullRejectsPush) {
  utils::mpsc_ring_buffer<int, 8> buf;

  for (int i = 0; i < 8; ++i) {
    ASSERT_TRUE (buf.try_push (i)) << "failed at i=" << i;
  }

  EXPECT_TRUE (buf.full ());
  EXPECT_FALSE (buf.try_push (99));
}

TEST (MpscRingBufferTest, EmptyPopReturnsNullopt) {
  utils::mpsc_ring_buffer<int, 8> buf;

  EXPECT_THAT (buf.try_pop (), Eq (std::nullopt));
}

TEST (MpscRingBufferTest, WrapAround) {
  utils::mpsc_ring_buffer<int, 8> buf;

  for (int round = 0; round < 10'000; ++round) {
    for (int i = 0; i < 8; ++i) {
      ASSERT_TRUE (buf.try_push (i));
    }
    for (int i = 0; i < 8; ++i) {
      auto value = buf.try_pop ();
      ASSERT_TRUE (value.has_value ());
      EXPECT_EQ (*value, i);
    }
  }

  EXPECT_TRUE (buf.empty ());
}

TEST (MpscRingBufferTest, MoveOnlyType) {
  utils::mpsc_ring_buffer<std::unique_ptr<int>, 4> buf;

  ASSERT_TRUE (buf.try_push (std::make_unique<int> (42)));
  auto value = buf.try_pop ();
  ASSERT_TRUE (value.has_value ());
  ASSERT_NE (*value, nullptr);
  EXPECT_EQ (**value, 42);
}

TEST (MpscRingBufferTest, PartialFillUpdatesSize) {
  utils::mpsc_ring_buffer<int, 8> buf;

  ASSERT_TRUE (buf.try_push (1));
  ASSERT_TRUE (buf.try_push (2));
  EXPECT_EQ (buf.size (), 2u);
  EXPECT_FALSE (buf.empty ());
  EXPECT_FALSE (buf.full ());

  auto value = buf.try_pop ();
  ASSERT_TRUE (value.has_value ());
  EXPECT_EQ (*value, 1);
  EXPECT_EQ (buf.size (), 1u);

  ASSERT_TRUE (buf.try_pop ().has_value ());
  EXPECT_TRUE (buf.empty ());
}

TEST (MpscRingBufferTest, MinimumCapacityFour) {
  utils::mpsc_ring_buffer<int, 4> buf;

  for (int round = 0; round < 1'000; ++round) {
    for (int i = 0; i < 4; ++i) {
      ASSERT_TRUE (buf.try_push (i));
    }
    for (int i = 0; i < 4; ++i) {
      auto value = buf.try_pop ();
      ASSERT_TRUE (value.has_value ());
      EXPECT_EQ (*value, i);
    }
  }
}

TEST (MpscRingBufferTest, MultiProducerSmallBufferContention) {
  utils::mpsc_ring_buffer<int, 4> buf;
  constexpr int kProducers = 4;
  constexpr int kPerProducer = 5'000;
  constexpr int kTotal = kProducers * kPerProducer;

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back ([&, p] {
      for (int i = 0; i < kPerProducer; ++i) {
        const int id = p * kPerProducer + i + 1;
        while (!buf.try_push (id)) {
        }
      }
    });
  }

  std::vector<int> seen;
  seen.reserve (kTotal);
  std::atomic<bool> failed {false};

  std::thread consumer ([&] {
    for (int n = 0; n < kTotal; ++n) {
      std::optional<int> value;
      do {
        value = buf.try_pop ();
      } while (!value);

      if (*value < 1 || *value > kTotal) {
        failed.store (true, std::memory_order_relaxed);
        return;
      }
      seen.push_back (*value);
    }
  });

  for (auto& t : producers) {
    t.join ();
  }
  consumer.join ();

  ASSERT_FALSE (failed.load (std::memory_order_relaxed));
  ASSERT_EQ (seen.size (), static_cast<std::size_t> (kTotal));
  EXPECT_EQ (std::set<int> (seen.begin (), seen.end ()).size (),
             static_cast<std::size_t> (kTotal));
  EXPECT_TRUE (buf.empty ());
}

TEST (MpscRingBufferTest, FullThenDrainAllowsReuse) {
  utils::mpsc_ring_buffer<int, 4> buf;

  for (int i = 0; i < 4; ++i) {
    ASSERT_TRUE (buf.try_push (i));
  }
  EXPECT_TRUE (buf.full ());
  EXPECT_FALSE (buf.try_push (99));

  for (int i = 0; i < 4; ++i) {
    auto value = buf.try_pop ();
    ASSERT_TRUE (value.has_value ());
    EXPECT_EQ (*value, i);
  }

  EXPECT_TRUE (buf.empty ());
  EXPECT_TRUE (buf.try_push (100));
  auto value = buf.try_pop ();
  ASSERT_TRUE (value.has_value ());
  EXPECT_EQ (*value, 100);
}

TEST (MpscRingBufferTest, SingleProducerStress) {
  utils::mpsc_ring_buffer<int, 1024> buf;
  constexpr int kCount = 1'000'000;
  std::atomic<bool> failed {false};

  std::thread producer ([&] {
    for (int i = 1; i <= kCount; ++i) {
      while (!buf.try_push (i)) {
      }
    }
  });

  std::thread consumer ([&] {
    for (int expected = 1; expected <= kCount; ++expected) {
      std::optional<int> value;
      do {
        value = buf.try_pop ();
      } while (!value);

      if (*value != expected) {
        failed.store (true, std::memory_order_relaxed);
        break;
      }
    }
  });

  producer.join ();
  consumer.join ();

  EXPECT_FALSE (failed.load (std::memory_order_relaxed));
  EXPECT_TRUE (buf.empty ());
}

TEST (MpscRingBufferTest, MultiProducerStress) {
  utils::mpsc_ring_buffer<int, 1024> buf;
  constexpr int kProducers = 8;
  constexpr int kPerProducer = 100'000;
  constexpr int kTotal = kProducers * kPerProducer;

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back ([&, p] {
      for (int i = 0; i < kPerProducer; ++i) {
        const int id = p * kPerProducer + i + 1;
        while (!buf.try_push (id)) {
        }
      }
    });
  }

  std::vector<int> seen;
  seen.reserve (kTotal);
  std::atomic<bool> failed {false};

  std::thread consumer ([&] {
    for (int n = 0; n < kTotal; ++n) {
      std::optional<int> value;
      do {
        value = buf.try_pop ();
      } while (!value);

      if (*value < 1 || *value > kTotal) {
        failed.store (true, std::memory_order_relaxed);
        return;
      }

      seen.push_back (*value);
    }
  });

  for (auto& t : producers) {
    t.join ();
  }
  consumer.join ();

  ASSERT_FALSE (failed.load (std::memory_order_relaxed));
  ASSERT_EQ (seen.size (), static_cast<std::size_t> (kTotal));

  const std::set<int> unique (seen.begin (), seen.end ());
  EXPECT_EQ (unique.size (), static_cast<std::size_t> (kTotal));
  EXPECT_TRUE (buf.empty ());
}

}  // namespace
