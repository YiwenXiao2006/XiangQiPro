// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Jiang.generated.h"

#define PATH_MI_CHESSMASK_RAD_JIANG TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_shuai.MI_ChessMask_rad_shuai'")
#define PATH_MI_CHESSMASK_BLACK_JIANG TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_jiang.MI_ChessMask_black_jiang'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Jiang : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Jiang();

	virtual void Init(EChessColor color, Position pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void Defeated() override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
