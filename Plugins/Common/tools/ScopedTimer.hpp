// ScopedTimer.hpp

#pragma once
/**
 * @file ScopedTimer.hpp
 * @brief Header file for timing utilities.
 */

#include <chrono>
#include <iostream>
#include <string>
#include "Config.hpp" // Include the config file to access the flags

#ifdef TIMING_MODE
class ScopedTimer {
public:
    ScopedTimer(const std::string &name) 
        : func_name(name), start(std::chrono::high_resolution_clock::now()) {}
    ~ScopedTimer() {
        auto stop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = stop - start;
        std::cout << func_name << " took " << duration.count() << " milliseconds." << std::endl;
    }

private:
    std::string func_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
#else
class ScopedTimer {
public:
    ScopedTimer(const std::string &) {}
    ~ScopedTimer() {}
};
#endif // TIMING_MODE
