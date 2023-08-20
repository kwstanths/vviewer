#include "TerminalColors.hpp"

namespace debug_tools {

    int GetWindowsColorCombination(TerminalColor fg_color, TerminalColor bg_color){
        return bg_color * 16 + fg_color;
    }

    int GetLinuxBackgroundColor(TerminalColor color){
        return color + 10;
    }
}

