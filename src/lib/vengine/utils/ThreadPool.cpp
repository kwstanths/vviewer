#include "ThreadPool.hpp"

#include "debug_tools/Console.hpp"

#include <mutex>

namespace vengine
{

ThreadPool::WorkerThread::WorkerThread(ThreadPool &threadpool)
    : m_threadpool(threadpool){};

ThreadPool::~ThreadPool()
{
    if (m_run)
        stop();
}

void ThreadPool::WorkerThread::loop()
{
    while (1) {
        /* Acquire lock */
        std::unique_lock<std::mutex> lock(m_threadpool.m_lock);

        /* Wait for work */
        m_threadpool.m_condition.wait(lock, [&]() { return !m_threadpool.m_tasks.empty() || !m_threadpool.m_run; });

        /* Exit if no work and exit is requested */
        if (m_threadpool.m_tasks.empty() && !m_threadpool.m_run) {
            lock.unlock();
            return;
        }

        /* Ignore if tasks empty */
        if (m_threadpool.m_tasks.empty()) {
            lock.unlock();
            continue;
        }

        /* Get a task */
        Task *task = m_threadpool.m_tasks.front();
        m_threadpool.m_tasks.pop();

        /* Unlock and run task */
        lock.unlock();

        task->run();
        task->signalReady();
    }
}

bool ThreadPool::init(uint32_t numThreads)
{
    m_run = true;
    for (uint32_t i = 0; i < numThreads; i++) {
        m_workerThreads.push_back(std::thread(&WorkerThread::loop, new WorkerThread(*this)));
    }

    m_initialized = true;
    return m_initialized;
}

Waitable *ThreadPool::push(Task *task)
{
    if (!m_initialized)
        return nullptr;

    if (task->parent() != nullptr)
        task->parent()->increaseSignalCount();

    std::lock_guard<std::mutex> lock(m_lock);
    m_tasks.push(task);
    m_condition.notify_one();

    return task;
}

void ThreadPool::stop()
{
    if (!m_initialized)
        return;

    /* Notify all threads */
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_run = false;
        m_condition.notify_all();
    }

    /* Wait for them to finish */
    for (std::vector<std::thread>::iterator itr = m_workerThreads.begin(); itr != m_workerThreads.end(); itr++)
        itr->join();

    m_initialized = false;
    return;
}

uint32_t ThreadPool::threads()
{
    return static_cast<uint32_t>(m_workerThreads.size());
}

}  // namespace vengine