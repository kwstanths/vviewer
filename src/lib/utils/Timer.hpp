#ifndef __Timer_HPP__
#define __Timer_HPP__

#include <sys/timeb.h>
#include <iostream>
#include <string>

#define __time_start(start) ftime(&start);
#define __time_stop(end) ftime(&end);

namespace debug_tools {

    class Timer {
        private:
            struct timeb start_, end_;

        public:
            /**
                Calls Start()
            */
            Timer();

            /**
              Store the start time 
             */
            void Start();

            /** 
              Store the stop time 
             */
            void Stop();

            /**
              Print the time difference of the stored values in milliseconds
             **/
            friend std::ostream & operator<<(std::ostream & stream, Timer const & R);

            /**
              Get the time difference of the stored values in milliseconds in a string 
              @return Elapsed time
             */
            std::string ToString();

            /**
              Get the time difference of the stoerd values in milliseconds in an integer
              @return Elapsed time
             */
            int64_t ToInt();

    };
}

#endif
