#include "stopwatch.h"
#include <chrono>
#include <iostream>

void Stopwatch::start(){
    start_time = std::chrono::high_resolution_clock::now();
}
void Stopwatch::stop(){
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time);
    int total = duration.count();
    int second = total/1000000;
    int milli = total/1000-second*1000;
    int micro = total%1000;
    std::cout<<second<<"s "<<milli<<"ms "<<micro<<"μs"<<std::endl; 
}

