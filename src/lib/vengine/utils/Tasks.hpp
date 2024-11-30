#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>
#include <mutex>
#include <condition_variable>

namespace vengine
{

class Task
{
public:
    Task()
    {
        m_waitLock = std::make_unique<std::mutex>();
        m_waitCondition = std::make_unique<std::condition_variable>();
    }

    /**
     * Override with task work.
     * [optional] Store current progress in progress
     */
    virtual bool work(float &progress) = 0;
    virtual float getProgress() const { return m_progress; }

    /* Wait on a task to finish */
    void wait()
    {
        std::unique_lock<std::mutex> lock(*m_waitLock);

        if (m_finished)
            return;

        m_waitCondition->wait(lock, [&]() { return m_finished == true; });
    }

    bool isSuccess() { return m_success; }
    bool isFinished() { return m_finished; }

    /* Run a task */
    bool run()
    {
        m_started = true;
        m_success = work(m_progress);
        m_finished = true;

        std::lock_guard<std::mutex> lock(*m_waitLock);
        m_waitCondition->notify_all();

        return m_success;
    }

protected:
    float m_progress = 0.0F;

private:
    bool m_started = false;
    bool m_finished = false;
    bool m_success = false;

    std::unique_ptr<std::mutex> m_waitLock;
    std::unique_ptr<std::condition_variable> m_waitCondition;
};

class TaskList
{
};

}  // namespace vengine

#endif