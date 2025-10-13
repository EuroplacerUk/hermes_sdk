#pragma once
#include <chrono>

namespace Hermes
{

    static inline std::chrono::system_clock::duration GetSeconds(double seconds)
    {
        using namespace std::chrono_literals;
        auto time = 1000ms * seconds;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time);
        return ms;
    }
}