#ifndef __ThreadPool_hpp_
#define __ThreadPool_hpp_

#include <vector>
#include <functional>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include "Tasks.hpp"

namespace vengine
{

class ThreadPool
{
public:
    ThreadPool(){};
    ~ThreadPool();

    /* Initialize thread pool */
    bool init(uint32_t numThreads);

    /* Push a task item */
    Waitable *push(Task *task);

    /* Request exit and wait */
    void stop();

    /* Get available workers */
    uint32_t threads();

private:
    class WorkerThread
    {
    public:
        WorkerThread(ThreadPool &threadpool);

        void loop();

    private:
        ThreadPool &m_threadpool;
    };

    std::vector<std::thread> m_workerThreads;

    std::queue<Task *> m_tasks;

    bool m_initialized = false;

    /* Tasks queue mutex and condition variable */
    std::mutex m_lock;
    std::condition_variable m_condition;

    std::atomic<bool> m_run;
};

}  // namespace vengine

#endif