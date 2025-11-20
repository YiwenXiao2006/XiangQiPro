// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Ma.h"

AChess_Ma::AChess_Ma()
{
	MyType = EChessType::MA;
}

void AChess_Ma::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::RED)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_MA));
	}
	else if (color == EChessColor::BLACK)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_MA));
	}
}

void AChess_Ma::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target)
{
	Super::GenerateMove2P(board2P, target);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GenerateMaMoves(Pos.X, Pos.Y, MyColor, Moves);

	GameState->ShowSettingPoint2P(Moves, this);
}
