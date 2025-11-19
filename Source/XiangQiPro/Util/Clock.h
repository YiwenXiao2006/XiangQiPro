// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

class XIANGQIPRO_API FClock
{
public:
    void Start();

    double GetElapsed() const;

    double GetElapsedMilliseconds() const;

    void Reset();

private:
    double StartTime = 0.0;
};