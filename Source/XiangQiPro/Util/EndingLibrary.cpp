// Copyright 2026 Ultimate Player All Rights Reserved.


#include "EndingLibrary.h"
#include "XiangQiPro/Util/ObjectManager.h"
#include "XiangQiPro/Util/Logger.h"

#define DATATABLE_PATH TEXT("/Script/Engine.DataTable'/Game/DataTable/ChessGenerationInfos.ChessGenerationInfos'")

TArray<FChessGenerationInfo> UEndingLibrary::GetChessGenerateInfo(int32 Index)
{
    FChessGenerationInfo Infos;
    auto DataTable = OM::GetObject<UDataTable>(DATATABLE_PATH);

    TArray<FName> rowName = DataTable->GetRowNames();

    if (Index < 0 || Index >= rowName.Num())
    {
        ULogger::LogError(TEXT("UEndingLibrary::GetChessGenerateInfo"), TEXT("Get generate information index out of bounds!"));
        return TArray<FChessGenerationInfo>();
    }

    FString ContextString;
    auto TableData = DataTable->FindRow<FChessGenerationInfos>(rowName[Index], ContextString, false);
    if (TableData)
    {
        return TableData->Infos;
    }
    else
    {
        ULogger::LogError(TEXT("UEndingLibrary::GetChessGenerateInfo"), "Can't find row by name!");
        return TArray<FChessGenerationInfo>();
    }
}
