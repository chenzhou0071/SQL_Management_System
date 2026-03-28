// src/server/network/ThreadPool.cpp
#include "ThreadPool.h"
#include "../../common/Logger.h"

namespace minisql {

ThreadPool::ThreadPool(size_t threadCount) {
    for (size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this] { workerThread(); });
    }
    LOG_INFO("ThreadPool", "Created " + std::to_string(threadCount) + " threads");
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::stop() {
    stop_.store(true);
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this] {
                return stop_.load() || !tasks_.empty();
            });

            if (stop_.load() && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

size_t ThreadPool::getQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex_));
    return tasks_.size();
}

}  // namespace minisql
