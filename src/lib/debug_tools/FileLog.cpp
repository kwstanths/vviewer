#include "FileLog.hpp"

namespace debug_tools {

    FileLog & FileLog::GetInstance(std::string file_name){
        static FileLog instance(file_name);
        return instance;
    }

    FileLog::~FileLog(){
        Close();
    }

    void FileLog::Log(DebugLevel level, std::string text){
        lock_.lock();

        log_file_ << GetStringTimestamp() << " [" << GetLevelInfo(level).first << "] : " << text << std::endl;
        log_file_.flush();
        lines_appended_++;

        lock_.unlock();
    }

    FileLog::FileLog(std::string file_name){
        log_file_.open(file_name, std::ios_base::out | std::ios_base::in | std::ios_base::trunc);
        if (!log_file_.is_open()) {
            log_file_.clear();
            log_file_.open(file_name , std::ios_base::out);
        }

    }

    void FileLog::Close(){
        log_file_.close();
    }
}
