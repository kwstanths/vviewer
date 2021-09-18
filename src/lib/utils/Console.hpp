#ifndef __Console_hpp__
#define __Console_hpp__

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <iostream>
#include <string.h>
#include <vector>

#include "Timestamp.hpp"
#include "TerminalColors.hpp"
#include "DebugLevels.hpp"

namespace utils {
    
    /**
        Print anything fast
        @param t What to print. Must have the << operator overloaded
    */
    template<typename T> void Console(T t) {
        std::cout << t << std::endl;
    }

    /**
        Print something to console
        @param importance The importance of the message
        @param text The text to print
        @param color The color of the message, Default color is dark white
    */
    void Console(DebugLevel level, std::string text, TerminalColor color = TerminalColor::DEF_FG);

    /**
        Print something to console as info
        @param text The text to print
    */
    void ConsoleInfo(std::string text);

    /**
        Print something to console as warning
        @param text The text to print
    */
    void ConsoleWarning(std::string text);

    /**
        Print something to console as fatal
        @param text The text to print
    */
    void ConsoleFatal(std::string text);
    
    /**
        Print something to console as critical
        @param text The text to print
    */
    void ConsoleCritical(std::string text);

    /**
        Custom print a string
        @param os The output stream
        @param text The string
        @param The output color. Default is dark white
    */
    void CustomPrint(std::ostream & os, std::string text, TerminalColor fg_color = TerminalColor::DEF_FG);

    /**
        Custom print the hex of a number
        @param os The output stream
        @param number The number
        @param color The output color. Default is dark white
    */
    void CustomPrintHex(std::ostream & os, int number, TerminalColor color = TerminalColor::DEF_FG);

    /**
        Custom print the hex of a number
        @param os The output stream
        @param number The number
        @param color The output color. Default is dark white
    */
    void CustomPrintHex(std::ostream & os, double number, TerminalColor color = TerminalColor::DEF_FG);

    /**
        Changes the color of the console
        @param color The color
    */
    void ChangeConsoleColor(TerminalColor fg_color);

    /**
        Restores the color of the console
    */
    void RestoreConsoleColor();

    /**
        Prints an 1d vector
        @param in The input vector
        @param orientation True means horizontal, false means vertical
    */    
    template<typename T> void Print1dVector(std::vector<T> & in, bool orientation = true){
        std::cout << "Vector size: " << in.size() << std::endl;
    
        for(size_t i=0; i<in.size(); i++)
            if (orientation) std::cout << "[" << i << "]:" << in[i] << " ";
            else std::cout << "[" << i << "]:" << in[i] << std::endl;
    
        if (orientation) std::cout << std::endl;
    }
	
    /**
        Prints two values in this form: "\t t: v"
    */
    template<typename Text, typename Value> void ConsoleInfo(Text t, Value v) {
    	std::cout << "\t" << t << " : " << v << std::endl;
    }
    
    /**
        Prints two values in this form "\t t: v" and continues the recursion to the rest arguments
        The rest of the arguments should be at least two, and always a multiple of two
    */
    template<typename Text, typename Value, typename ... Args> void ConsoleInfo(Text t, Value v, Args ... args) {
        std::cout << "\t" << t << " : " << v << std::endl;
        ConsoleInfo(args...);
    }
    
    /**
        Prints a custom info box in this form
        Timestamp [DebugLevel] - text
            arg1: arg2
            ... rest
                    
        The rest of the arguments should be at least two, and always a multiple of two
    */
    template<typename ... Args> void ConsoleInfoL(DebugLevel level, std::string text, Args ... args) {
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(level);
        std::cout << GetStringTimestamp() << " [";
        CustomPrint(std::cout, level_info.first, level_info.second);
        std::cout << "] : " << text << std::endl;
        ConsoleInfo(args...);
    }

    /**
        Wait for anything to be typed
    */
    void WaitInput();
}


#endif
