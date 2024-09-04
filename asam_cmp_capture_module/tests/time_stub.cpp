#include "include/time_stub.h"

TimeStub::TimeStub()
{
    startTime = std::chrono::steady_clock::now();
    auto startAbsTime = std::chrono::system_clock::now();
    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch());
}

std::chrono::microseconds TimeStub::getMicroSecondsSinceDeviceStart() const
{
    auto currentTime = std::chrono::steady_clock::now();
    auto microSecondsSinceDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
    return microSecondsSinceDeviceStart;
}

std::chrono::microseconds TimeStub::getMicroSecondsFromEpochToDeviceStart() const
{
    return microSecondsFromEpochToDeviceStart;
}
