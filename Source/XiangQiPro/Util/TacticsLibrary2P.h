// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/ChessInfo.h"
#include "../Util/ChessMove.h"
#include "../GameObject/ChessBoard2P.h"

#include "CoreMinimal.h"
#include "TacticsLibrary2P.generated.h"

class AChesses;
class UChessBoard2P;

/**
 * 中国象棋战术库类
 * 实现各种经典象棋战术的检测和应用
 */
UCLASS()
class XIANGQIPRO_API UTacticsLibrary2P : public UObject
{
    GENERATED_BODY()

public:
    UTacticsLibrary2P();

    // 设置棋盘引用
    void SetBoard(TWeakObjectPtr<UChessBoard2P> newBoard);

    // 战术检测主函数
    bool DetectTactics(const FChessMove2P& move, EChessColor color, FString& tacticName, int32& tacticScore);

    // 将帅面对面战术检测
    bool DetectKingFaceOff(const FChessMove2P& move, EChessColor color, int32& score);

    // 单个战术检测函数
    bool DetectFork(const FChessMove2P& move, EChessColor color, int32& score);           // 捉双/捉多子
    bool DetectPin(const FChessMove2P& move, EChessColor color, int32& score);           // 牵制
    bool DiscoveredAttack(const FChessMove2P& move, EChessColor color, int32& score);    // 闪击
    bool Skewer(const FChessMove2P& move, EChessColor color, int32& score);              // 串打
    bool DoubleCheck(const FChessMove2P& move, EChessColor color, int32& score);         // 双将
    bool Sacrifice(const FChessMove2P& move, EChessColor color, int32& score);           // 弃子
    bool DefensiveTactic(const FChessMove2P& move, EChessColor color, int32& score);     // 防守战术
    bool PositionalTactic(const FChessMove2P& move, EChessColor color, int32& score);    // 局面性战术

    // 工具函数
    bool IsPieceValuable(EChessType pieceType);
    int32 GetPieceValue(EChessType pieceType);
    bool IsPositionAttacked(int32 x, int32 y, EChessColor attackerColor);
    bool CanPieceCapture(int32 fromX, int32 fromY, int32 targetX, int32 targetY, EChessColor attackerColor);
    TArray<FChessMove2P> GetThreatsAfterMove(const FChessMove2P& move, EChessColor color);

private:
    TWeakObjectPtr<UChessBoard2P> board2P;

    // 棋子基础价值表
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };
};