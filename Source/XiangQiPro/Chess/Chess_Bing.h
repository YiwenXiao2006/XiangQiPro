// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chesses.h"
#include "Chess_Bing.generated.h"

#define PATH_MI_CHESSMASK_RAD_BING TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/red/MI_ChessMask_rad_bing.MI_ChessMask_rad_bing'")
#define PATH_MI_CHESSMASK_BLACK_BING TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Decal/ChessMask/black/MI_ChessMask_black_zu.MI_ChessMask_black_zu'")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AChess_Bing : public AChesses
{
	GENERATED_BODY()

public:

	AChess_Bing();

	virtual void Init(EChessColor color, Position pos, TWeakObjectPtr<UChessBoard2P> board2P) override;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target) override;
	
};
