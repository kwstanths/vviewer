#include "TerminalColors.hpp"

namespace utils {

    int GetWindowsColorCombination(TerminalColor fg_color, TerminalColor bg_color){
        return bg_color * 16 + fg_color;
    }

    int GetLinuxBackgroundColor(TerminalColor color){
        return color + 10;
    }
}

