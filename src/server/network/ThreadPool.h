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

// ============================================================
// 线程池 - 任务队列 + 工作线程
// ============================================================
// 线程池原理：
// 1. 创建固定数量的工作线程（如4个）
// 2. 维护任务队列（tasks_）
// 3. 工作线程从队列取任务执行
// 4. 使用条件变量同步：队列空时线程等待，有任务时唤醒
//
// 优势：
// - 避免频繁创建/销毁线程的开销
// - 控制并发线程数量，防止资源耗尽
// - 任务异步执行，不阻塞调用线程
//
// 任务提交流程：
// 1. submit()提交任务（包装为packaged_task）
// 2. 加入任务队列，通知一个工作线程
// 3. 返回future，调用者可等待结果
//
// 线程池关闭流程：
// 1. 设置stop_标志
// 2. 通知所有工作线程
// 3. 工作线程退出循环
// 4. join所有线程
// ============================================================
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool();

    // 提交任务到线程池
    // 返回future，调用者可等待任务完成并获取结果
    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())>;

    // 停止线程池（不再接受新任务）
    void stop();

    size_t getQueueSize() const;

private:
    void workerThread();  // 工作线程函数

    std::vector<std::thread> workers_;              // 工作线程列表
    std::queue<std::function<void()>> tasks_;        // 任务队列
    std::mutex queueMutex_;                          // 队列互斥锁
    std::condition_variable condition_;              // 条件变量（同步）
    std::atomic<bool> stop_{false};                  // 停止标志
};

// ============================================================
// 提交任务（模板函数）
// ============================================================
// 参数：F - 可调用对象（函数、lambda、bind表达式等）
// 返回：future<return_type> - 可等待任务结果
//
// 实现步骤：
// 1. 将任务包装为packaged_task（支持future）
// 2. 加入任务队列（加锁）
// 3. 通知一个工作线程（condition_.notify_one）
// 4. 返回future，调用者可get()等待结果
//
// packaged_task作用：
// - 包装可调用对象，支持异步执行
// - 通过get_future()获取future
// - future.get()返回任务结果或异常
// ============================================================
template<typename F>
auto ThreadPool::submit(F&& f) -> std::future<decltype(f())> {
    using return_type = decltype(f());  // 任务返回类型

    // 包装任务为shared_ptr<packaged_task>（支持复制）
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));

    {
        std::lock_guard<std::mutex> lock(queueMutex_);  // 加锁保护队列
        tasks_.emplace([task]() { (*task)(); });        // 加入队列（lambda包装）
    }

    condition_.notify_one();  // 通知一个工作线程有新任务
    return task->get_future();  // 返回future，调用者可等待结果
}

}  // namespace minisql
