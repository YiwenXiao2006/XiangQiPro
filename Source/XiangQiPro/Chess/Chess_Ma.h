// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Ma.generated.h"

#define PATH_MI_CHESSMASK_RAD_MA TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_ma.MI_ChessMask_rad_ma'")
#define PATH_MI_CHESSMASK_BLACK_MA TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_ma.MI_ChessMask_black_ma'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Ma : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Ma();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
