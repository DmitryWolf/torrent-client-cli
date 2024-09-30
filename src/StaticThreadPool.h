#pragma once

#include <iostream>
#include <atomic>
#include <functional>
#include <thread>
#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <assert.h>
#include <condition_variable>
#include <future>

// Unbounded Blocking Multi-Producer/Multi-Consumer (MPMC) Queue

// 1) shared state
// 2) mutex
// 3) predicate(state)

template <typename T>
class UnboundedBlockingMPMCQueue{
public:
    // Thread role: producer
    void Put(T value);
    
    // Thread role: consumer
    T Take();
private:
    T TakeLocked();

    std::deque<T> buffer_; // Guarded by mutex_
    std::mutex mutex_;
    std::condition_variable not_empty_;
};

class StaticThreadPool{
    using Task = std::function<void()>;
public:
    StaticThreadPool(size_t workers);
    ~StaticThreadPool();
 
    void Submit(Task task);

    void Join();

private:
    void StartWorkerThreads(size_t count);
    
    void WorkerRoutine();

    std::vector<std::thread> workers_;
    UnboundedBlockingMPMCQueue<Task> tasks_;
};