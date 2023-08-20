#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>

namespace vengine {

struct Task {
    std::function<bool(float&)> f;

    bool started = false;
    bool finished = false;
    bool success = false;
    
    float progress = 0.0F;

    bool operator () () {
        started = true;
        success = f(progress);
        finished = true;
        return success;
    }

    virtual float getProgress() const { return progress; }
};

}

#endif