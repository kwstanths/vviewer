#ifndef __Levels_hpp__
#define __Levels_hpp__

#include <utility>
#include <string>

#include "TerminalColors.hpp"

namespace debug_tools {

    /** Type of debugging */
    enum DebugLevel {
        INFO,
        WARNING,
        CRITICAL,
        FATAL,
    };

    /**
      Get a string and a color based on the importance 
      @param importance The Type of warning
      @return A string of the Type, and a color for the Type
     */
    std::pair<std::string, TerminalColor> GetLevelInfo(DebugLevel level);

}


#endif
