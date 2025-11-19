// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Clock.h"

void FClock::Start()
{
    StartTime = FPlatformTime::Seconds();
}

double FClock::GetElapsed() const
{
    return FPlatformTime::Seconds() - StartTime;
}

double FClock::GetElapsedMilliseconds() const
{
    return GetElapsed() * 1000.0;
}

void FClock::Reset()
{
    Start();
}
