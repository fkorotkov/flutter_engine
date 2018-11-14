// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include "flutter/fml/task_runner.h"

#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/message_loop_impl.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/thread.h"

namespace fml {

TaskRunner::TaskRunner(fml::RefPtr<MessageLoopImpl> loop)
    : loop_impl_(std::move(loop)) {}

TaskRunner::~TaskRunner() = default;

void TaskRunner::PostTask(fml::closure task) {
  loop_impl_->PostTask(std::move(task), fml::TimePoint::Now());
}

void TaskRunner::PostTaskForTime(fml::closure task,
                                 fml::TimePoint target_time) {
  loop_impl_->PostTask(std::move(task), target_time);
}

void TaskRunner::PostDelayedTask(fml::closure task, fml::TimeDelta delay) {
  loop_impl_->PostTask(std::move(task), fml::TimePoint::Now() + delay);
}

bool TaskRunner::RunsTasksOnCurrentThread() {
  if (!fml::MessageLoop::IsInitializedForCurrentThread()) {
    return false;
  }
  return MessageLoop::GetCurrent().GetLoopImpl() == loop_impl_;
}

void TaskRunner::RunNowOrPostTask(fml::closure task) {
  FML_DCHECK(task);

  if (RunsTasksOnCurrentThread()) {
    task();
    return;
  }

  // Legacy path for platforms on which we do not have access to the message
  // loop implementation (Fuchsia and Desktop Linux).
  if (!loop_impl_) {
    AutoResetWaitableEvent latch;
    PostTask([task, &latch]() {
      task();
      latch.Signal();
    });
    latch.Wait();
    return;
  }

  FML_CHECK(false);

  // MessageLoop::GetCurrent().RunNested(
  //     [runner = Ref(this), task](auto outer_activation) {
  //       runner->PostTask([runner, task, outer_activation]() {
  //         MessageLoop::GetCurrent().RunNested(
  //             [task, outer_activation](auto inner_activation) {
  //               inner_activation->Terminate();
  //               FML_LOG(INFO) << "Performing task.";
  //               task();
  //               outer_activation->Terminate();
  //             });
  //       });
  //     });
}

}  // namespace fml
