// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Logger.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(XQPro, Log, All);

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API ULogger : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	static void Log(int32 i);

	static void Log(FString t, int32 i);

	static void Log(FString t, float f);

	static void Log(FString t, FString s);

	static void Log(FString s);

	static void LogWarning(FString s);

	static void LogWarning(FString t, FString s);

	static void LogError(FString t);

	static void LogError(FString t, FString s);

	static void LogError(FString t, int32 i);

};
