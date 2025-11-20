// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chess_Jiang.h"

AChess_Jiang::AChess_Jiang() : Super()
{
	MyType = EChessType::JIANG;
}

void AChess_Jiang::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	Super::Init(color, pos, board2P);
	if (color == EChessColor::RED)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_RAD_JIANG));
	}
	else if (color == EChessColor::BLACK)
	{
		ChessMask->SetDecalMaterial(OM::GetObject<UMaterialInterface>(PATH_MI_CHESSMASK_BLACK_JIANG));
	}
}

void AChess_Jiang::Defeated()
{
	Super::Defeated();
	GameState->GameOver(MyColor == EChessColor::BLACK ? EChessColor::RED : EChessColor::BLACK); // 通知游戏状态，游戏获胜
}

void AChess_Jiang::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target)
{
	Super::GenerateMove2P(board2P, target);

	// 获取所有移动方式
	TArray<FChessMove2P> Moves;
	board2P->GenerateJiangMoves(Pos.X, Pos.Y, MyColor, Moves);
	
	GameState->ShowSettingPoint2P(Moves, this);
}
