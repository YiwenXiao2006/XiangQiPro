// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XiangQiPro/Util/ChessInfo.h"
#include "XiangQiPro/Util/ChessMove.h"
#include "XiangQiPro/Chess/AllChessHeader.h"
#include "EndingLibrary.generated.h"


USTRUCT(BlueprintType)
struct FChessGenerationInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AChesses> Class;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChessColor Color;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Pos;

	FChessGenerationInfo() : Class(nullptr), Color(EChessColor::REDCHESS), Pos(Position(0, 0))
	{ }

	FChessGenerationInfo(TSubclassOf<AChesses> InClass, EChessColor InColor, Position InPos) : Class(InClass), Color(InColor), Pos(InPos)
	{ }
};

USTRUCT(BlueprintType)
struct FChessGenerationInfos : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FChessGenerationInfo> Infos;
};

/**
 * ²Ð¾Ö¿â
 */
UCLASS()
class XIANGQIPRO_API UEndingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure)
	static TArray<FChessGenerationInfo> GetChessGenerateInfo(int32 Index);
};
