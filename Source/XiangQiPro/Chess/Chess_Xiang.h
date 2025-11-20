// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Xiang.generated.h"

#define PATH_MI_CHESSMASK_RAD_XIANG TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_xiang.MI_ChessMask_rad_xiang'")
#define PATH_MI_CHESSMASK_BLACK_XIANG TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_xiang.MI_ChessMask_black_xiang'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Xiang : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Xiang();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
