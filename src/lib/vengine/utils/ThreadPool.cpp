#include "ThreadPool.hpp"

#include <mutex>

namespace vengine
{

ThreadPool::WorkerThread::WorkerThread(ThreadPool &threadpool)
    : m_threadpool(threadpool){};

void ThreadPool::WorkerThread::loop()
{
    while (1) {
        /* Wait for work */
        std::unique_lock<std::mutex> lock(m_threadpool.m_lock);
        m_threadpool.m_condition.wait(lock, [&]() { return !m_threadpool.m_tasks.empty() || !m_threadpool.m_run; });

        /* Exit if requested */
        if (!m_threadpool.m_run) 
        {
            lock.unlock();
            return;
        }

        /* Ignore if tasks empty */
        if (m_threadpool.m_tasks.empty()) {
            lock.unlock();
            continue;
        }

        /* Get a task */
        Task* task = m_threadpool.m_tasks.front();
        m_threadpool.m_tasks.pop();

        /* Unlock and execure */
        lock.unlock();

        (*task)();
    }
}

ThreadPool::ThreadPool(uint32_t numThreads)
{
    m_run = true;
    for (uint32_t i = 0; i < numThreads; i++) {
        m_workerThreads.push_back(std::thread(&WorkerThread::loop, new WorkerThread(*this)));
    }
}

ThreadPool::~ThreadPool()
{
    if (m_run)
        stop();
}

void ThreadPool::push(Task *task)
{
    std::lock_guard<std::mutex> lock(m_lock);
    m_tasks.push(task);
    m_condition.notify_one();
}

void ThreadPool::stop()
{
    m_run = false;

    m_condition.notify_all();
    for (std::vector<std::thread>::iterator itr = m_workerThreads.begin(); itr != m_workerThreads.end(); itr++)
        itr->join();

    return;
}

}  // vengine