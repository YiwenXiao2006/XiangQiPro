// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Xiang.h"

AChess_Xiang::AChess_Xiang()
{
	MyType = EChessType::XIANG;
}

void AChess_Xiang::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::RED)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_XIANG));
	}
	else if (color == EChessColor::BLACK)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_XIANG));
	}
}

void AChess_Xiang::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target)
{
	Super::GenerateMove2P(board2P, target);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GenerateXiangMoves(Pos.X, Pos.Y, MyColor, Moves);

	GameState->ShowSettingPoint2P(Moves, this);
}
