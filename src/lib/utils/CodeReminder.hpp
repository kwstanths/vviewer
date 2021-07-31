#ifndef __CodeReminder_hpp__
#define __CodeReminder_hpp__


#include "Console.hpp"


#define CodeReminder(text) {\
    std::cout << "Reminder: " + std::string(#text) + " : " + std::string(__FILE__) + ":" + std::to_string(__LINE__) << std::endl; \
    }

#define CodeReminderFatal(text) \
    { \
        std::cout << "Reminder: " + std::string(#text) + " : " + std::string(__FILE__) + ":" + std::to_string(__LINE__) << std::endl; \
        debug_tools::WaitInput(); \
        exit(-1); \
    }


#endif