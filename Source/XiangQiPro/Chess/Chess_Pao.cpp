// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Pao.h"

AChess_Pao::AChess_Pao()
{
	MyType = EChessType::PAO;
}

void AChess_Pao::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::RED)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_PAO));
	}
	else if (color == EChessColor::BLACK)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_PAO));
	}
}

void AChess_Pao::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::GenerateMove2P(board2P);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GeneratePaoMoves(Pos.X, Pos.Y, MyColor, Moves);

	GameState->ShowSettingPoint2P(Moves, this);
}
