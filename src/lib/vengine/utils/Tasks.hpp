#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>
#include <mutex>
#include <iostream>
#include <condition_variable>

namespace vengine
{

class Waitable
{
    friend class ThreadPool;

public:
    Waitable()
        : m_parent(nullptr)
        , m_signalCount(1)
    {
        m_waitLock = std::make_unique<std::mutex>();
        m_waitCondition = std::make_unique<std::condition_variable>();
    }

    /**
     * @brief Construct a new Waitable object with a parent waitable that needs to wait for this one to finish before the parent
     * finishes
     *
     * @param parent
     */
    Waitable(Waitable *parent)
        : m_parent(parent)
        , m_signalCount(1)
    {
        m_waitLock = std::make_unique<std::mutex>();
        m_waitCondition = std::make_unique<std::condition_variable>();
    }

    /* Wait for signal ready, don't use this object after the wait finishes */
    void wait()
    {
        std::unique_lock<std::mutex> lock(*m_waitLock);

        if (m_signalCount == 0)
            return;

        m_waitCondition->wait(lock, [&]() { return m_signalCount == 0; });
    }

private:
    std::unique_ptr<std::mutex> m_waitLock;
    std::unique_ptr<std::condition_variable> m_waitCondition;
    uint32_t m_signalCount = 1;
    Waitable *m_parent = nullptr;

    Waitable *parent() { return m_parent; }

    void signalReady()
    {
        std::lock_guard<std::mutex> lock(*m_waitLock);
        m_signalCount--;

        if (m_signalCount != 0)
            return;

        // notify parent task if you have one
        if (m_parent != nullptr)
            m_parent->signalReady();

        // notify waiters as well
        m_waitCondition->notify_all();
    }

    void increaseSignalCount()
    {
        std::lock_guard<std::mutex> lock(*m_waitLock);
        m_signalCount++;
    }
};

class Task : public Waitable
{
    friend class ThreadPool;

public:
    Task()
        : Waitable(){};
    Task(Task *parentTask)
        : Waitable(parentTask){};

    /**
     * Override with task work.
     * [optional] Store current progress in progress
     */
    virtual bool work(float &progress) = 0;
    virtual float getProgress() const { return m_progress; }

    bool isSuccess() { return m_success; }
    bool isFinished() { return m_finished; }

    /* Run a task */
    bool run()
    {
        m_started = true;
        m_success = work(m_progress);
        m_finished = true;
        return m_success;
    }

protected:
    float m_progress = 0.0F;

private:
    bool m_started = false;
    bool m_finished = false;
    bool m_success = false;
};

}  // namespace vengine

#endif