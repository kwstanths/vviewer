#ifndef __Timer_HPP__
#define __Timer_HPP__

#include <iostream>
#include <string>
#include <chrono>

namespace debug_tools {

    class Timer {
        private:
            std::chrono::high_resolution_clock::time_point m_start;
            std::chrono::high_resolution_clock::time_point m_end;

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
            std::string ToString() const;

            /**
              Get the time difference of the stoerd values in milliseconds in an integer
              @return Elapsed time
             */
            int64_t ToInt() const;

    };
}

#endif
