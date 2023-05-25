#pragma once

#include <chrono>
#define TIME(X, ...)                                                           \
  X.start();                                                                   \
  __VA_ARGS__                                                                  \
  X.stop()

class Stopwatch{
    public:
        void start();
        void stop();

    private:
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
};
