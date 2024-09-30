#include "StaticThreadPool.h"

// UnboundedBlockingMPMCQueue Implementation
template <typename T>
void UnboundedBlockingMPMCQueue<T>::Put(T value) {
    std::lock_guard<std::mutex> guard(mutex_);
    buffer_.push_back(std::move(value));
    not_empty_.notify_one();
}

template <typename T>
T UnboundedBlockingMPMCQueue<T>::Take() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (buffer_.empty()){ // 
        // 1) Release mutex, 2) Wait 3) Reacquire mutex
        // Spurious wakeup
        not_empty_.wait(lock);
    }
    return TakeLocked();
}

template <typename T>
T UnboundedBlockingMPMCQueue<T>::TakeLocked() {
    assert(!buffer_.empty());
    T front = std::move(buffer_.front());
    buffer_.pop_front();
    return front;
}

// StaticThreadPool Implementation
StaticThreadPool::StaticThreadPool(size_t workers) {
    StartWorkerThreads(workers);
}

StaticThreadPool::~StaticThreadPool() {
    assert(workers_.empty());
}

void StaticThreadPool::Submit(Task task) {
    tasks_.Put(std::move(task));
}

void StaticThreadPool::Join() {
    for (auto& worker : workers_) {
        tasks_.Put({});  // Poison pill
    }
    for (auto& worker : workers_) {
        worker.join();
    }
    workers_.clear();
}

void StaticThreadPool::StartWorkerThreads(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        workers_.emplace_back([this]() {
            WorkerRoutine();
        });
    }
}

void StaticThreadPool::WorkerRoutine() {
    while (true) {
        auto task = tasks_.Take();
        if (!task) {
            break;
        }
        task();
    }
}

// Explicit instantiation of UnboundedBlockingMPMCQueue template for Task type
template class UnboundedBlockingMPMCQueue<std::function<void()>>;
