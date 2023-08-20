#ifndef __Timestamp_hpp__
#define __Timestamp_hpp__

#include <iostream>
#include <chrono>
#include <string>

namespace debug_tools {

    /** A timestamp struct */
    typedef struct Timestamp{
        size_t year;
        size_t month;
        size_t day;
        size_t hour;
        size_t minutes;
        size_t sec;
        size_t msec;
        Timestamp(int y, int m, int d, int h, int min, int s, int ms): 
            year(y), month(m), day(d), hour(h), minutes(min), sec(s), msec(ms){};

    } Timestamp_t;

    /**
      Get a string timestamp with hours, minutes, seconds and milliseconds
      @return A timestamp in a string
     */
    std::string GetStringTimestamp();

    /**
        Get a full string timestamp with year, month, day, hours, minutes, seconds, milliseconds
        @return A timestamp ina a string
    */
    std::string GetStringTimestampFull();

    /**
      Get a current timestamp
      @return A timestamp struct
     */
    Timestamp_t GetFullTimestamp();

    /**
        Get the amount of seconds passed since epoch
        @return Seconds
    */
    uint64_t GetSecondsSinceEpoch();

    /**
        Get the amount of milliseconds passed since epoch
        @return Milliseconds
    */
    uint64_t GetMSecondsSinceEpoch();
}

#endif
