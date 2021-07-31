#include "DebugLevels.hpp"


namespace utils {

    std::pair<std::string, TerminalColor> GetLevelInfo(DebugLevel level){
        switch (level) {
            case DebugLevel::INFO:
                {
                    return std::make_pair<std::string, TerminalColor>("Info", TerminalColor::DARK_GREEN);
                }
            case DebugLevel::WARNING:
                {
                    return std::make_pair<std::string, TerminalColor>("Warning", TerminalColor::DARK_YELLOW);
                }
            case DebugLevel::CRITICAL:
                {
                    return std::make_pair<std::string, TerminalColor>("Critical", TerminalColor::YELLOW);
                }
            case DebugLevel::FATAL:
                {
#ifdef _WIN32
                    return std::make_pair<std::string, TerminalColor>("Fatal", TerminalColor::RED);
#endif // _WIN32

                    return std::make_pair<std::string, TerminalColor>("Fatal", TerminalColor::DARK_RED);
                }
        }

        return std::make_pair<std::string, TerminalColor>("Unknown error", TerminalColor::RED);
    }
}
