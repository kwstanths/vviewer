#include "Timer.hpp"

namespace utils {
    Timer::Timer() {
        Start();
    }

    void Timer::Start() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    void Timer::Stop() {
        m_end = std::chrono::high_resolution_clock::now();
    }

    std::string Timer::ToString() const 
    {
        return std::to_string(ToInt());
    }

    int64_t Timer::ToInt() const 
    {
        std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start);
        return diff.count();
    }

    std::ostream & operator<<(std::ostream & stream, Timer const & R){
        return stream << R.ToInt();
    }

}
