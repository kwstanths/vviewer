#include "Timestamp.hpp"


namespace utils {

    std::string GetStringTimestamp(){

        time_t current_time = time(NULL);
#ifdef _WIN32
        struct tm * tm = (struct tm *)malloc(sizeof(struct tm));
		localtime_s(tm, &current_time);
#elif __linux__
        struct tm * tm;
        tm = localtime(&current_time);
#endif

        char date[64];
        /* 
           %Y : year
           %m : month
           %d : day
           %H : hour
           %M : minute
           %S : second
         */
        strftime(date, sizeof(date), "%H-%M-%S", tm);

        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch());

        int64_t milliseconds = ms.count() % 1000;
        std::string ret = std::string(date) + "-" + std::to_string(milliseconds);
        if (milliseconds < 100) ret += " "; /* Padding for better alignment */
        if (milliseconds < 10) ret += " "; 

        return ret;
    }

    std::string GetStringTimestampFull() {
        time_t current_time = time(NULL);
#ifdef _WIN32
        struct tm * tm = (struct tm *) malloc(sizeof(struct tm));
        localtime_s(tm, &current_time);
#elif __linux__
        struct tm * tm;
        tm = localtime(&current_time);
#endif

        char date[64];
        /*
        %Y : year
        %m : month
        %d : day
        %H : hour
        %M : minute
        %S : second
        */
        strftime(date, sizeof(date), "%Y/%m/%d %H-%M-%S", tm);

        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch());

        int64_t milliseconds = ms.count() % 1000;
        std::string ret = std::string(date) + "-" + std::to_string(milliseconds);
        if (milliseconds < 100) ret += " "; /* Padding for better alignment */
        if (milliseconds < 10) ret += " ";

        return ret;
    }

    Timestamp_t GetFullTimestamp(){
        time_t current_time = time(NULL);
#ifdef _WIN32
        struct tm * tm = (struct tm *) malloc(sizeof(struct tm));
		localtime_s(tm, &current_time);
#elif __linux__
        struct tm * tm;
        tm = localtime(&current_time);
#endif

        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch());

        return Timestamp_t(tm->tm_year + 1900, 
                tm->tm_mon+1, 
                tm->tm_mday, 
                tm->tm_hour, 
                tm->tm_min, 
                tm->tm_sec + 1, 
                ms.count() % 1000);

    }

    uint64_t GetSecondsSinceEpoch(){
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()); 
        return now.count();
    }

    uint64_t GetMSecondsSinceEpoch(){
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); 
        return now.count();
    }

}
