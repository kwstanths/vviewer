#ifndef __FileLog__hpp__
#define __FileLog_hpp__

#include <fstream>
#include <mutex>

#include "Timestamp.hpp"
#include "TerminalColors.hpp"
#include "DebugLevels.hpp"

namespace utils {

    class FileLog {
        public:

            /**
              Get a FileLog instance. If none exists, create a new one with the filename
              @param file_name The name of the log file
              @return A static FileLog instance
             */
            static FileLog & GetInstance(std::string file_name);

            /**
              Delete copy constructor. Don't make copies of the object
             */
            FileLog(FileLog const &) = delete;

            /**
              Delete assignment operator/ Don't make copies of the object
             */
            void operator=(FileLog const &) = delete;

            /**
              When the object is destroyed (At program exit because of static), close the file
             */
            ~FileLog();

            /**
              Appends a line in the file, with a timestamp in the front
              @param text The text to append to the file
             */
            void Log(DebugLevel level, std::string text);

        private:

            /* Holds the file object */
            std::fstream log_file_;

            /* Lock to protect concurrent access to the log file */
            std::mutex lock_;

            /* Holds the total lines appended to the file */
            unsigned long int lines_appended_;

            /**
              Create a log file, if we already have one, override the data inside the file
              @param file_name The name of the file
             */
            FileLog(std::string file_name);

            /**
              Close the file
             */
            void Close();
    };

}

#endif
