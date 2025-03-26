#ifndef _DURATION_H_
#define _DURATION_H_

#include <chrono>

using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using SystemTimePoint = std::chrono::time_point<std::chrono::system_clock>;

using namespace std::chrono_literals;

constexpr std::chrono::hours operator"" _days(unsigned long long days)
{
    return std::chrono::hours(days * 24);
}

#endif // _DURATION_H_
