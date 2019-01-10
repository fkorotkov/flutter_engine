// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_TASK_RUNNER_H_
#define FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_TASK_RUNNER_H_

#include <memory>

#include "flutter/fml/macros.h"
#include "flutter/fml/task_runner.h"

namespace shell {

class EmbedderTask {
 public:
  explicit EmbedderTask(fml::closure task) : task_(task) {}

  ~EmbedderTask() = default;

  void Perform() {
    if (!task_) {
      return;
    }
    task_();
    task_ = nullptr;
  }

 private:
  fml::closure task_;

  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderTask);
};

class EmbedderTaskRunner final : public fml::TaskRunner {
 public:
  struct DispatchTable {
    std::function<void(std::unique_ptr<EmbedderTask> task,
                       fml::TimePoint target_time)>
        post_task;

    std::function<bool(void)> runs_tasks_on_current_thread;

    bool IsValid() const;
  };

  explicit EmbedderTaskRunner(DispatchTable table);

  ~EmbedderTaskRunner() override;

  void PostTask(fml::closure task) override;

  void PostDelayedTask(fml::closure task, fml::TimeDelta delay) override;

  void PostTaskForTime(fml::closure task, fml::TimePoint target_time) override;

  bool RunsTasksOnCurrentThread() override;

 private:
  DispatchTable dispatch_table_;

  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderTaskRunner);
};

}  // namespace shell

#endif  // FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_TASK_RUNNER_H_
