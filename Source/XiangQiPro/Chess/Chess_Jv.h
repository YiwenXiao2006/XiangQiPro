// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Jv.generated.h"

#define PATH_MI_CHESSMASK_RAD_JV TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_jv.MI_ChessMask_rad_jv'")
#define PATH_MI_CHESSMASK_BLACK_JV TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_jv.MI_ChessMask_black_jv'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Jv : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Jv();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P) override;
	
};
