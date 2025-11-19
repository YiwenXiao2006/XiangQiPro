// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Logger.h"

DEFINE_LOG_CATEGORY(XQPro);

void ULogger::Log(int32 i)
{
	UE_LOG(XQPro, Log, TEXT("%d"), i);
}

void ULogger::Log(FString t, int32 i)
{
	UE_LOG(XQPro, Log, TEXT("%s: %d"), *t, i);
}

void ULogger::Log(FString t, float f)
{
	UE_LOG(XQPro, Log, TEXT("%s: %f"), *t, f);
}

void ULogger::Log(FString t, FString s)
{
	UE_LOG(XQPro, Log, TEXT("%s: %s"), *t, *s);
}

void ULogger::Log(FString s)
{
	UE_LOG(XQPro, Log, TEXT("%s"), *s);
}

void ULogger::LogWarning( FString s)
{
	UE_LOG(XQPro, Warning, TEXT("%s"), *s);
}

void ULogger::LogWarning(FString t, FString s)
{
	UE_LOG(XQPro, Warning, TEXT("%s: %s"), *t, *s);
}

void ULogger::LogError(FString t)
{
	UE_LOG(XQPro, Error, TEXT("%s"), *t);
}

void ULogger::LogError(FString t, FString s)
{
	UE_LOG(XQPro, Error, TEXT("%s: %s"), *t, *s);
}

void ULogger::LogError(FString t, int32 i)
{
	UE_LOG(XQPro, Error, TEXT("%s: %d"), *t, i);
}
