// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/ChessInfo.h"
#include "../Util/ChessMove.h"
#include "../Util/Clock.h"

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI2P.generated.h"

class UChessBoard2P;
class AChesses;

typedef UAI2P AI2P;
typedef UKismetMathLibrary Math;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API UAI2P : public UObject
{
	GENERATED_BODY()

private:

    FClock Timer;

    TWeakObjectPtr<UChessBoard2P> board2P;

	// 搜索深度
	int32 maxDepth;

    // 时间限制（毫秒）
    int32 timeLimit = 5000;

    // 历史启发表
    int32 historyTable[10][9][10][9]; // [fromX][fromY][toX][toY]

public:

    UAI2P();

    // 设置搜索深度
    void SetDepth(int32 Depth);

    // 设置棋盘状态
    void SetBoard(TWeakObjectPtr<UChessBoard2P> newBoard);

    // 评估函数
    int32 Evaluate();

    // 位置价值评估
    void InitializePositionScores(int32 positionScores[2][10][9]);

    // 行动力评估
    int32 EvaluateMobility(EChessColor color);

    // 威胁评估
    int32 EvaluateThreats(EChessColor color);

    // 静态走法评估函数
    int32 EvaluateMove(const FChessMove2P& move) const;

    // 检查是否超时
    bool IsTimeOut();

    // 极小化极大算法
    int32 Minimax(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer);

    // 静态搜索（Quiescence Search）避免水平线效应
    int32 QuiescenceSearch(int32 alpha, int32 beta, bool maximizingPlayer);

    TArray<FChessMove2P> GenerateCaptureMoves(EChessColor color);

    void SortMoves(TArray<FChessMove2P>& moves);

    void SortMovesWithHistory(TArray<FChessMove2P>& moves, int32 depth);

    void UpdateHistoryTable(const FChessMove2P& move, int32 depth);

    // 获取AI的最佳走法
    TWeakObjectPtr<AChesses> GetBestMove(FChessMove2P& bestMove);
};
