#include "Console.hpp"

namespace debug_tools {

    void Console(DebugLevel level, std::string text, TerminalColor color){
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(level);
        if (level == DebugLevel::INFO) {
            std::cout << GetStringTimestamp() << " [";
            CustomPrint(std::cout, level_info.first, level_info.second);
            std::cout << "]: ";
            CustomPrint(std::cout, text, color);
            std::cout << std::endl;
        } else {
            std::cerr << GetStringTimestamp() << " [";
            CustomPrint(std::cerr, level_info.first, level_info.second);
            std::cerr << "]: ";
            CustomPrint(std::cout, text, color);
            std::cout << std::endl;
        }

    }

    void ConsoleInfo(std::string text)
    {
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(DebugLevel::INFO);
        std::cout << GetStringTimestamp() << " [";
        CustomPrint(std::cout, level_info.first, level_info.second);
        std::cout << "]: ";
        CustomPrint(std::cout, text, TerminalColor::DEF_FG);
        std::cout << std::endl;
    }

    void ConsoleWarning(std::string text)
    {
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(DebugLevel::WARNING);
        std::cout << GetStringTimestamp() << " [";
        CustomPrint(std::cout, level_info.first, level_info.second);
        std::cout << "]: ";
        CustomPrint(std::cout, text, TerminalColor::DEF_FG);
        std::cout << std::endl;
    }

    void ConsoleFatal(std::string text)
    {
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(DebugLevel::FATAL);
        std::cout << GetStringTimestamp() << " [";
        CustomPrint(std::cout, level_info.first, level_info.second);
        std::cout << "]: ";
        CustomPrint(std::cout, text, TerminalColor::DEF_FG);
        std::cout << std::endl;
    }

    void ConsoleCritical(std::string text)
    {
        std::pair<std::string, TerminalColor> level_info = GetLevelInfo(DebugLevel::CRITICAL);
        std::cout << GetStringTimestamp() << " [";
        CustomPrint(std::cout, level_info.first, level_info.second);
        std::cout << "]: ";
        CustomPrint(std::cout, text, TerminalColor::DEF_FG);
        std::cout << std::endl;
    }

    void CustomPrint(std::ostream & os, std::string text, TerminalColor fg_color){

        os.precision(16);

        ChangeConsoleColor(fg_color);

        os << text;

        RestoreConsoleColor();

    } 

    void CustomPrintHex(std::ostream & os, int number, TerminalColor color){

        ChangeConsoleColor(color);

        os << std::hex << number << std::dec << std::endl;

        RestoreConsoleColor();
    } 

    void CustomPrintHex(std::ostream & os, double number, TerminalColor color){

        ChangeConsoleColor(color);

        os << std::hexfloat << number << std::defaultfloat << std::endl;

        RestoreConsoleColor();
    }

    void ChangeConsoleColor(TerminalColor fg_color){
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fg_color);
#endif
#ifdef __linux__
        std::cout << "\033[0;" << fg_color << "m" << std::flush;        
#endif
    }

    void RestoreConsoleColor(){
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), TerminalColor::DEF_FG);
#endif
#ifdef __linux__
        std::cout << "\033[0;" << TerminalColor::DEF_FG << "m" << std::flush;
#endif 
    }

    void WaitInput() {
        std::cout << "Type Enter to continue..." << std::endl;
        std::cin.ignore();
    }

}

