// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Bing.h"

AChess_Bing::AChess_Bing()
{
	MyType = EChessType::BING;
}

void AChess_Bing::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::RED)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_BING));
	}
	else if (color == EChessColor::BLACK)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_BING));
	}
}

void AChess_Bing::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::GenerateMove2P(board2P);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GenerateBingMoves(Pos.X, Pos.Y, MyColor, Moves);

	GameState->ShowSettingPoint2P(Moves, this);
}
