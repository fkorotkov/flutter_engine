// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/message_loop.h"

#include <utility>

#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/fml/message_loop_impl.h"
#include "flutter/fml/task_runner.h"
#include "flutter/fml/thread.h"
#include "flutter/fml/thread_local.h"

namespace fml {

FML_THREAD_LOCAL ThreadLocal tls_message_loop([](intptr_t value) {
  delete reinterpret_cast<MessageLoop*>(value);
});

MessageLoop& MessageLoop::GetCurrent() {
  auto* loop = reinterpret_cast<MessageLoop*>(tls_message_loop.Get());
  FML_CHECK(loop != nullptr)
      << "MessageLoop::EnsureInitializedForCurrentThread was not called on "
         "this thread prior to message loop use.";
  return *loop;
}

void MessageLoop::EnsureInitializedForCurrentThread() {
  if (tls_message_loop.Get() != 0) {
    // Already initialized.
    return;
  }
  auto* message_loop_ptr = new MessageLoop();
  tls_message_loop.Set(reinterpret_cast<intptr_t>(message_loop_ptr));
}

bool MessageLoop::IsInitializedForCurrentThread() {
  return tls_message_loop.Get() != 0;
}

MessageLoop::MessageLoop()
    : loop_impl_(MessageLoopImpl::Create()),
      task_runner_(MakeRefCounted<TaskRunner>(loop_impl_)) {
  FML_CHECK(loop_impl_);
}

MessageLoop::~MessageLoop() = default;

void MessageLoop::Run() {
  loop_impl_->DoRun();
}

void MessageLoop::RunNested(
    std::function<void(RefPtr<TaskRunner>, RefPtr<MessageLoopImpl>)>
        on_nested_task_runner) {
  FML_DCHECK(on_nested_task_runner);

  auto nested_loop_impl = MessageLoopImpl::Create();
  FML_CHECK(nested_loop_impl);

  auto nested_task_runner = MakeRefCounted<TaskRunner>(nested_loop_impl);
  nested_task_runner->PostTask(
      [nested_task_runner, on_nested_task_runner, nested_loop_impl]() {
        on_nested_task_runner(nested_task_runner, nested_loop_impl);
      });

  // Run the nested loop. It is now the job of the the caller to terminate the
  // activation using the task runner we just gave it.
  nested_loop_impl->DoRun();

  // The nested loop is done. Re-arm timers.
  loop_impl_->RunExpiredTasksNow();
}

void MessageLoop::Terminate() {
  loop_impl_->DoTerminate();
}

RefPtr<TaskRunner> MessageLoop::GetTaskRunner() {
  return task_runner_;
}

RefPtr<MessageLoopImpl> MessageLoop::GetLoopImpl() const {
  return loop_impl_;
}

void MessageLoop::AddTaskObserver(intptr_t key, closure callback) {
  loop_impl_->AddTaskObserver(key, callback);
}

void MessageLoop::RemoveTaskObserver(intptr_t key) {
  loop_impl_->RemoveTaskObserver(key);
}

void MessageLoop::RunExpiredTasksNow() {
  loop_impl_->RunExpiredTasksNow();
}

}  // namespace fml
