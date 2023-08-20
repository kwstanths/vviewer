#ifndef __Assert_hpp__
#define __Assert_hpp__

#include "Console.hpp"

#define ASSERTIONS_ENABLED

#ifdef ASSERTIONS_ENABLED
    
    #ifdef _MSC_VER
        #define debug_break() __asm { int 3 };
    #elif __GNUC__ 
        #define debug_break() __asm__ ("int 3");
    #endif

    #define _assert(expr)    \
    if (expr) { }   \
    else {  \
        debug_tools::Console(debug_tools::FATAL, "Assertion failed: " + std::string(#expr) + ", " + std::string(__FILE__) + ":" + std::to_string(__LINE__));   \
        /* debug_break(); */  \
        debug_tools::WaitInput(); \
        exit(-1); \
    }

#else
    #define _assert(expr)
#endif




#endif
