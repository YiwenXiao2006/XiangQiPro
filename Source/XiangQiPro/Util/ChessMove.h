// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessMove.generated.h"

typedef FVector2D Position;

 // 走法结构
USTRUCT(BlueprintType)
struct FChessMove2P
{
    GENERATED_USTRUCT_BODY()
    Position from;
    Position to;
    int32 score;  // 用于评估走法的得分

    FChessMove2P(Position f = Position(), Position t = Position(), int32 s = 0) : from(f), to(t), score(s)
    {
    }

    bool operator<(const FChessMove2P& other) const
    {
        return score < other.score;
    }
};

UCLASS()
class XIANGQIPRO_API UMove : public UObject
{
    GENERATED_BODY()
};
