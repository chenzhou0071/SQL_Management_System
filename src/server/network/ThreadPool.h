// src/server/network/ThreadPool.h
#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>

namespace minisql {

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool();

    // 提交任务
    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())>;

    // 停止线程池
    void stop();

    size_t getQueueSize() const;

private:
    void workerThread();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};

template<typename F>
auto ThreadPool::submit(F&& f) -> std::future<decltype(f())> {
    using return_type = decltype(f());

    auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return task->get_future();
}

}  // namespace minisql
