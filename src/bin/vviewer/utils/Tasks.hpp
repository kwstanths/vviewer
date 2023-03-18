#ifndef __Tasks_hpp__
#define __Tasks_hpp__

#include <functional>

struct TaskWaitableUI {
    std::function<bool()> f;
    bool finished = false;

    bool operator () () {
        bool ret = f();
        finished = true;
        return ret;
    }

    bool hasfinished() const { return finished; }
    virtual float getProgress() const { return 0.0F; }
};


#endif