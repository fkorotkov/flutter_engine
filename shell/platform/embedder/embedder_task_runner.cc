// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_task_runner.h"

#include <utility>

#include "flutter/fml/message_loop_impl.h"

namespace shell {

EmbedderTaskRunner::EmbedderTaskRunner(DispatchTable table)
    : fml::TaskRunner(nullptr), dispatch_table_(std::move(table)) {
  FML_CHECK(dispatch_table_.IsValid());
}

EmbedderTaskRunner::~EmbedderTaskRunner() = default;

bool EmbedderTaskRunner::DispatchTable::IsValid() const {
  return post_task && runs_tasks_on_current_thread;
}

void EmbedderTaskRunner::PostTask(fml::closure task) {
  PostTaskForTime(task, fml::TimePoint::Now());
}

void EmbedderTaskRunner::PostDelayedTask(fml::closure task,
                                         fml::TimeDelta delay) {
  PostTaskForTime(task, fml::TimePoint::Now() + delay);
}

void EmbedderTaskRunner::PostTaskForTime(fml::closure task,
                                         fml::TimePoint target_time) {
  dispatch_table_.post_task(std::make_unique<EmbedderTask>(task), target_time);
}

bool EmbedderTaskRunner::RunsTasksOnCurrentThread() {
  return dispatch_table_.runs_tasks_on_current_thread();
}

}  // namespace shell
