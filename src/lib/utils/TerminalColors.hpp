#ifndef __Colors_hpp__
#define __Colors_hpp__

#ifdef _WIN32
#define COLOR_DEF_BG          0      
#define COLOR_DARK_BLUE       1
#define COLOR_DARK_GREEN      2
#define COLOR_DARK_CYAN       3
#define COLOR_DARK_RED        4
#define COLOR_DARK_PURPLE     5
#define COLOR_DARK_YELLOW     6
#define COLOR_DARK_WHITE      7
#define COLOR_DEF_FG          COLOR_DARK_WHITE
#define COLOR_GREY            8
#define COLOR_BLUE            9
#define COLOR_GREEN           10
#define COLOR_CYAN            11
#define COLOR_RED             12
#define COLOR_PURPLE          13
#define COLOR_YELLOW          14
#define COLOR_WHITE           15
#elif __linux__
#define COLOR_DARK_BLUE       34
#define COLOR_DARK_GREEN      32
#define COLOR_DARK_CYAN       36
#define COLOR_DARK_RED        31
#define COLOR_DARK_PURPLE     35
#define COLOR_DARK_YELLOW     33
#define COLOR_DARK_WHITE      37
#define COLOR_DEF_FG          39
#define COLOR_DEF_BG          39
#define COLOR_GREY            90
#define COLOR_BLUE            94
#define COLOR_GREEN           92
#define COLOR_CYAN            96
#define COLOR_RED             91
#define COLOR_PURPLE          95
#define COLOR_YELLOW          93
#define COLOR_WHITE           97
#endif


namespace utils {
    /** TerminalColor of console output */
    enum TerminalColor {
        DARK_BLUE = COLOR_DARK_BLUE,
        DARK_GREEN = COLOR_DARK_GREEN,
        DARK_CYAN = COLOR_DARK_CYAN,
        DARK_RED = COLOR_DARK_RED,
        DARK_PURPLE = COLOR_DARK_PURPLE,
        DARK_YELLOW = COLOR_DARK_YELLOW,
        DARK_WHITE = COLOR_DARK_WHITE,
        GREY = COLOR_GREY,
        BLUE = COLOR_BLUE,
        GREEN = COLOR_GREEN,
        CYAN = COLOR_CYAN,
        RED = COLOR_RED,
        PURPLE = COLOR_PURPLE,
        YELLOW = COLOR_YELLOW,
        WHITE = COLOR_WHITE,
        DEF_FG = COLOR_DEF_FG,
        DEF_BG = COLOR_DEF_BG,
    }; 

    /**
      Get the color combination for a windows system for background and foreground color
      @param fg_color The foreground color
      @param bg_color The background color
      @return The color combination
     */
    int GetWindowsColorCombination(TerminalColor fg_color, TerminalColor bg_color);

    /**
        Get the background color code for linux
        @param TerminalColor color
        @return The background color code
    */
    int GetLinuxBackgroundColor(TerminalColor color);
}

#endif
