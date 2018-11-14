// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include <atomic>
#include <thread>

#include "flutter/fml/message_loop.h"
#include "flutter/fml/synchronization/count_down_latch.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/task_runner.h"
#include "flutter/fml/thread.h"
#include "flutter/fml/thread_local.h"
#include "flutter/testing/testing.h"

namespace fml {

TEST(TaskRunnerTest, RunNowOrPostTaskOnDifferentThread) {
  Thread::SetCurrentThreadName("thread1");

  FML_THREAD_LOCAL ThreadLocal count;
  Thread thread("thread2");

  AutoResetWaitableEvent latch;
  thread.GetTaskRunner()->PostTask([&latch]() {
    count.Set(999);
    latch.Signal();
  });
  latch.Wait();
  ASSERT_NE(count.Get(), 999);

  bool did_assert = false;

  MessageLoop::EnsureInitializedForCurrentThread();

  thread.GetTaskRunner()->RunNowOrPostTask([&did_assert]() {
    FML_LOG(INFO) << "Here";
    ASSERT_EQ(count.Get(), 999);
    did_assert = true;
  });

  ASSERT_TRUE(did_assert);
}

TEST(TaskRunnerTest, RunNowOrPostTaskOnSameThread) {
  FML_THREAD_LOCAL ThreadLocal count;
  Thread thread;

  thread.GetTaskRunner()->PostTask([]() { count.Set(999); });

  ASSERT_NE(count.Get(), 999);

  size_t assertions_checked = 0;

  AutoResetWaitableEvent latch;
  thread.GetTaskRunner()->PostTask([&assertions_checked, &latch]() {
    ASSERT_EQ(count.Get(), 999);
    assertions_checked++;
    latch.Signal();

    MessageLoop::GetCurrent().GetTaskRunner()->RunNowOrPostTask(
        [&assertions_checked]() {
          ASSERT_EQ(count.Get(), 999);
          assertions_checked++;
        });
  });

  latch.Wait();
  ASSERT_EQ(assertions_checked, 2u);
}

class CheckpointLogger {
 public:
  CheckpointLogger() = default;
  ~CheckpointLogger() = default;

  bool Assert(size_t checkpoint, size_t line) {
    std::lock_guard<std::mutex> lock(mutex_);
    checkpoints_.push_back(++sequence_);
    FML_LOG(INFO) << "Checkpoint: " << sequence_
                  << " Thread: " << Thread::GetCurrentThreadName()
                  << " Line: " << line;
    return checkpoints_.back() == checkpoint;
  }

 private:
  std::mutex mutex_;
  std::size_t sequence_ = 0;
  std::vector<size_t> checkpoints_;
  FML_DISALLOW_COPY_AND_ASSIGN(CheckpointLogger);
};

TEST(TaskRunnerTest, TasksCanBePostedOnASyncLockedRunner) {
  CheckpointLogger checkpoint;

  Thread::SetCurrentThreadName("thread1");

  Thread thread2("thread2"), thread3("thread3");

  MessageLoop::EnsureInitializedForCurrentThread();

  RefPtr<TaskRunner> runner2, runner3;

  runner2 = thread2.GetTaskRunner();
  runner3 = thread3.GetTaskRunner();

  EXPECT_TRUE(checkpoint.Assert(1, __LINE__));
  runner2->RunNowOrPostTask([&]() {
    EXPECT_TRUE(checkpoint.Assert(2, __LINE__));
    runner3->RunNowOrPostTask([&]() {
      EXPECT_TRUE(checkpoint.Assert(3, __LINE__));
      runner2->RunNowOrPostTask(
          [&]() { EXPECT_TRUE(checkpoint.Assert(4, __LINE__)); });
      EXPECT_TRUE(checkpoint.Assert(5, __LINE__));
    });
    EXPECT_TRUE(checkpoint.Assert(6, __LINE__));
  });
  EXPECT_TRUE(checkpoint.Assert(7, __LINE__));
}

}  // namespace fml
