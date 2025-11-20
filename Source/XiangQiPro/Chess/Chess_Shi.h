// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Shi.generated.h"

#define PATH_MI_CHESSMASK_RAD_SHI TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_shi.MI_ChessMask_rad_shi'")
#define PATH_MI_CHESSMASK_BLACK_SHI TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_shi.MI_ChessMask_black_shi'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Shi : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Shi();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
