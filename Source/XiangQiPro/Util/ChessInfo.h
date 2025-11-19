// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessInfo.generated.h"

    // 棋子类型
UENUM(BlueprintType)
enum class EChessType : uint8
{
    EMPTY = 0 UMETA(DisplayName = "空"),
    JIANG = 1 UMETA(DisplayName = "将/帅"),  // 将/帅
    SHI = 2 UMETA(DisplayName = "士/仕"),  // 士/仕
    XIANG = 3 UMETA(DisplayName = "象/相"), // 象/相
    MA = 4 UMETA(DisplayName = "马/傌"),    // 马/傌
    JV = 5 UMETA(DisplayName = "车/俥"),  // 车/俥
    PAO = 6 UMETA(DisplayName = "炮/炮"),   // 炮/炮
    BING = 7 UMETA(DisplayName = "兵/卒")   // 兵/卒
};

// 棋子颜色
UENUM(BlueprintType)
enum class EChessColor : uint8
{
    RED = 0 UMETA(DisplayName = "红方"),
    BLACK = 1 UMETA(DisplayName = "黑方"),
    GREEN = 2 UMETA(DisplayName = "绿方")
};

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API UChessInfo : public UObject
{
    GENERATED_BODY()
};