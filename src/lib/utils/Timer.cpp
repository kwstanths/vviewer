#include "Timer.hpp"

namespace debug_tools {
    Timer::Timer() {
        Start();
    }

    void Timer::Start() {
        __time_start(start_);
    }

    void Timer::Stop() {
        __time_stop(end_);
    }

    std::string Timer::ToString() {
        double value = (1000.0 * (end_.time - start_.time) + (end_.millitm - start_.millitm));
        return std::to_string(value);
    }

    int64_t Timer::ToInt() {
        return (1000 * (end_.time - start_.time) + (end_.millitm - start_.millitm));
    }

    std::ostream & operator<<(std::ostream & stream, Timer const & R){
        return stream << (1000.0 * (R.end_.time - R.start_.time) + (R.end_.millitm - R.start_.millitm));
    }

}
