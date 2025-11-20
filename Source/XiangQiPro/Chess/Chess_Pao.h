// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Pao.generated.h"

#define PATH_MI_CHESSMASK_RAD_PAO TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_pao.MI_ChessMask_rad_pao'")
#define PATH_MI_CHESSMASK_BLACK_PAO TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_pao.MI_ChessMask_black_pao'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Pao : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Pao();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
