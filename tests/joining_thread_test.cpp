#include <utils/joining_thread.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

namespace {

TEST (JoiningThreadTest, JoinsOnDestruction) {
  std::atomic<bool> started {false};
  std::atomic<bool> finished {false};

  {
    utils::joining_thread worker ([&] {
      started.store (true, std::memory_order_release);
      std::this_thread::sleep_for (std::chrono::milliseconds {10});
      finished.store (true, std::memory_order_release);
    });

    while (!started.load (std::memory_order_acquire)) {
      std::this_thread::yield ();
    }
    EXPECT_FALSE (finished.load (std::memory_order_acquire));
  }

  EXPECT_TRUE (finished.load (std::memory_order_acquire));
}

TEST (JoiningThreadTest, ExceptionInScopeStillJoins) {
  std::atomic<bool> finished {false};

  try {
    utils::joining_thread worker ([&] {
      std::this_thread::sleep_for (std::chrono::milliseconds {10});
      finished.store (true, std::memory_order_release);
    });
    throw std::runtime_error {"abort scope"};
  } catch (const std::runtime_error&) {
  }

  EXPECT_TRUE (finished.load (std::memory_order_acquire));
}

TEST (JoiningThreadTest, ExplicitJoinBeforeDestruction) {
  std::atomic<bool> finished {false};

  {
    utils::joining_thread worker (
        [&] { finished.store (true, std::memory_order_release); });
    worker.join ();
    EXPECT_FALSE (worker.joinable ());
  }

  EXPECT_TRUE (finished.load (std::memory_order_acquire));
}

TEST (JoiningThreadTest, DetachLeavesNonJoinable) {
  std::atomic<bool> started {false};

  {
    utils::joining_thread worker ([&] {
      started.store (true, std::memory_order_release);
      std::this_thread::sleep_for (std::chrono::milliseconds {50});
    });

    while (!started.load (std::memory_order_acquire)) {
      std::this_thread::yield ();
    }

    worker.detach ();
    EXPECT_FALSE (worker.joinable ());
  }

  std::this_thread::sleep_for (std::chrono::milliseconds {100});
}

TEST (JoiningThreadTest, MoveConstructTransfersOwnership) {
  std::atomic<bool> finished {false};

  utils::joining_thread first ([&] {
    std::this_thread::sleep_for (std::chrono::milliseconds {10});
    finished.store (true, std::memory_order_release);
  });

  utils::joining_thread second {std::move (first)};
  EXPECT_FALSE (first.joinable ());
  EXPECT_TRUE (second.joinable ());
}

TEST (JoiningThreadTest, MoveAssignJoinsPrevious) {
  std::atomic<bool> first_finished {false};

  utils::joining_thread first ([&] {
    std::this_thread::sleep_for (std::chrono::milliseconds {50});
    first_finished.store (true, std::memory_order_release);
  });

  utils::joining_thread second;

  first = std::move (second);

  EXPECT_TRUE (first_finished.load (std::memory_order_acquire));
  EXPECT_FALSE (first.joinable ());
}

TEST (JoiningThreadTest, DefaultConstructedIsNotJoinable) {
  utils::joining_thread worker;
  EXPECT_FALSE (worker.joinable ());
}

TEST (JoiningThreadTest, GetIdMatchesRunningThread) {
  std::atomic<std::thread::id> worker_id {};

  {
    utils::joining_thread worker ([&] {
      worker_id.store (std::this_thread::get_id (), std::memory_order_release);
    });

    while (worker_id.load (std::memory_order_acquire) == std::thread::id {}) {
      std::this_thread::yield ();
    }

    EXPECT_EQ (worker.get_id (), worker_id.load (std::memory_order_acquire));
  }
}

}  // namespace
