// Copyright 2026 Ultimate Player All Rights Reserved.


#include "SaveGameLibrary.h"
#include "SaveGameProgress.h"

#include <Kismet/GameplayStatics.h>
#include "XiangQiPro/Util/Logger.h"

USaveGameProgress* USaveGameLibrary::GetSaveGameProgress(const FString& SlotName, int32 UserIndex)
{
    if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return Cast<USaveGameProgress>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    }
    // 不存在时创建新存档
    return Cast<USaveGameProgress>(UGameplayStatics::CreateSaveGameObject(USaveGameProgress::StaticClass()));
}

bool USaveGameLibrary::ApplySaveGameProgress(USaveGameProgress* SaveGameInstance, const FString& SlotName, int32 UserIndex)
{
    if (SaveGameInstance)
    {
        return UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, UserIndex);
    }
    return false;
}

int32 USaveGameLibrary::GetEndingGameLevel()
{
    USaveGameProgress* SaveGameInstance = GetSaveGameProgress(TEXT("GameProgress"), 0);
    if (SaveGameInstance)
    {
        return SaveGameInstance->EndingGameLevel;
    }
    else
    {
        ULogger::LogError(TEXT("USaveGameLibrary::GetEndingGameLevel"), TEXT("SaveGameInstance is nullptr!"));
        return 0;
    }
}

int32 USaveGameLibrary::GetEndingGameLevel_Max()
{
    USaveGameProgress* SaveGameInstance = GetSaveGameProgress(TEXT("GameProgress"), 0);
    if (SaveGameInstance)
    {
        return SaveGameInstance->EndingGameLevel_Max;
    }
    else
    {
        ULogger::LogError(TEXT("USaveGameLibrary::GetEndingGameLevel_Max"), TEXT("SaveGameInstance is nullptr!"));
        return 0;
    }
}

void USaveGameLibrary::SetEndingGameLevel(int32 EndingGameLevel)
{
    USaveGameProgress* SaveGameInstance = GetSaveGameProgress(TEXT("GameProgress"), 0);
    if (SaveGameInstance)
    {
        SaveGameInstance->EndingGameLevel = EndingGameLevel;
        ApplySaveGameProgress(SaveGameInstance, TEXT("GameProgress"), 0);
    }
    else
    {
        ULogger::LogError(TEXT("USaveGameLibrary::SetEndingGameLevel"), TEXT("SaveGameInstance is nullptr!"));
    }
}

void USaveGameLibrary::SetEndingGameLevel_Max(int32 EndingGameLevel_Max)
{
    USaveGameProgress* SaveGameInstance = GetSaveGameProgress(TEXT("GameProgress"), 0);
    if (SaveGameInstance)
    {
        SaveGameInstance->EndingGameLevel_Max = EndingGameLevel_Max;
        ApplySaveGameProgress(SaveGameInstance, TEXT("GameProgress"), 0);
    }
    else
    {
        ULogger::LogError(TEXT("USaveGameLibrary::SetEndingGameLevel"), TEXT("SaveGameInstance is nullptr!"));
    }
}

void USaveGameLibrary::UpdateEndingGameLevelData()
{
    int32 level = GetEndingGameLevel();
    int32 level_max = GetEndingGameLevel_Max();

    // 相等则可以更新最大解锁的关卡
    if (level == level_max)
    {
        SetEndingGameLevel(level + 1);
        SetEndingGameLevel_Max(level_max + 1);
    }
    else
    {
        SetEndingGameLevel(level + 1);
    }
}
