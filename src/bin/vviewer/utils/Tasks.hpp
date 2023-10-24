#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>

namespace vengine
{

struct Task {
    std::function<bool(float &)> function;

    bool started = false;
    bool finished = false;
    bool success = false;

    float progress = 0.0F;

    bool operator()()
    {
        started = true;
        success = function(progress);
        finished = true;
        return success;
    }

    virtual float getProgress() const { return progress; }

    Task(){};
    Task(std::function<bool(float &)> &funct)
        : function(funct){};
    Task(std::function<bool(float &)> funct)
        : function(funct){};
};

}  // namespace vengine

#endif