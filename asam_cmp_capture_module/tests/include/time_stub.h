#pragma once
#include <chrono>

class TimeStub
{
public:
    TimeStub();
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;
    std::chrono::microseconds getMicroSecondsFromEpochToDeviceStart() const;
    
private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;
};