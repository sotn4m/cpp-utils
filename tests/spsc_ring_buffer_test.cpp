#include <utils/spsc_ring_buffer.hpp>

#include <atomic>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

namespace {

TEST (SpscRingBufferTest, EmptyOnConstruction) {
  utils::spsc_ring_buffer<int, 8> buf;

  EXPECT_TRUE (buf.empty ());
  EXPECT_FALSE (buf.full ());
  EXPECT_EQ (buf.size (), 0u);
  EXPECT_EQ (buf.capacity (), 8u);
}

TEST (SpscRingBufferTest, PushPopFifo) {
  utils::spsc_ring_buffer<int, 8> buf;

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

TEST (SpscRingBufferTest, FullRejectsPush) {
  utils::spsc_ring_buffer<int, 8> buf;

  for (int i = 0; i < 8; ++i) {
    ASSERT_TRUE (buf.try_push (i)) << "failed at i=" << i;
  }

  EXPECT_TRUE (buf.full ());
  EXPECT_FALSE (buf.try_push (99));
}

TEST (SpscRingBufferTest, EmptyPopReturnsNullopt) {
  utils::spsc_ring_buffer<int, 8> buf;

  EXPECT_FALSE (buf.try_pop ().has_value ());
}

TEST (SpscRingBufferTest, WrapAround) {
  utils::spsc_ring_buffer<int, 8> buf;

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

TEST (SpscRingBufferTest, MoveOnlyType) {
  utils::spsc_ring_buffer<std::unique_ptr<int>, 4> buf;

  ASSERT_TRUE (buf.try_push (std::make_unique<int> (42)));
  auto value = buf.try_pop ();
  ASSERT_TRUE (value.has_value ());
  ASSERT_NE (*value, nullptr);
  EXPECT_EQ (**value, 42);
}

TEST (SpscRingBufferTest, SpscStress) {
  utils::spsc_ring_buffer<int, 1024> buf;
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

}  // namespace
