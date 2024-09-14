#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>
#include <mutex>

namespace vengine
{

struct Task {
public:
    Task(std::function<bool(float &)> &funct)
        : m_function(funct){};
    Task(std::function<bool(float &)> funct)
        : m_function(funct){};

    bool operator()()
    {
        m_started = true;
        m_success = m_function(m_progress);
        m_finished = true;
        m_waitCondition.notify_all();
        return m_success;
    }

    virtual float getProgress() const { return m_progress; }

    void wait() 
    {
        std::unique_lock<std::mutex> lock(m_waitLock);
        m_waitCondition.wait(lock, [&]() { return m_finished == true; });
    }

    bool isSuccess() { return m_success; }
    bool isFinished() { return m_finished; }

private:
    std::function<bool(float &)> m_function;

    bool m_started = false;
    bool m_finished = false;
    bool m_success = false;

    std::mutex m_waitLock;
    std::condition_variable m_waitCondition;

    float m_progress = 0.0F;
};

}  // namespace vengine

#endif