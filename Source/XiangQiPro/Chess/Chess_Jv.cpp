// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Jv.h"

AChess_Jv::AChess_Jv()
{
	MyType = EChessType::JV;
}

void AChess_Jv::Init(EChessColor color, Position pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::REDCHESS)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_JV));
	}
	else if (color == EChessColor::BLACKCHESS)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_JV));
	}

	if (color == EChessColor::REDCHESS)
	{
		MyName = UTF8_TO_TCHAR("俥");
	}
	else
	{
		MyName = UTF8_TO_TCHAR("車");
	}
}

void AChess_Jv::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target)
{
	Super::GenerateMove2P(board2P, target);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GenerateJvMoves(Pos.X, Pos.Y, MyColor, Moves);

	GameState->ShowSettingPoint2P(Moves, this);
}
