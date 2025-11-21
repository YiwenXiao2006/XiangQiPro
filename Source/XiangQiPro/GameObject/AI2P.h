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

    // 立刻停止思考
    void StopThinkingImmediately();

    // 评估函数
    int32 Evaluate(EChessColor color = EChessColor::BLACK);

    // 位置价值评估
    void InitializePositionScores(int32 positionScores[2][10][9]);

    // 行动力评估
    int32 EvaluateMobility(EChessColor color);

    // 威胁评估
    int32 EvaluateThreats(EChessColor color);

    // 评估将/帅安全性
    int32 EvaluateKingSafety(EChessColor color);

    // 静态走法评估函数
    int32 EvaluateMove(const FChessMove2P& move);

    // 评估士的走法
    int32 EvaluateShiMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> shi);

    // 评估象的走法
    int32 EvaluateXiangMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> xiang);

    // 评估进攻棋子的走法（适当降低优先级）
    int32 EvaluateAttackMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> attacker);

    // 评估走法后的位置价值
    int32 EvaluatePositionValue(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess);

    // 评估士的位置价值
    int32 EvaluateShiPosition(int32 x, int32 y, EChessColor color);

    // 评估象的位置价值
    int32 EvaluateXiangPosition(int32 x, int32 y, EChessColor color);

    // 检查象眼是否通畅
    bool IsXiangEyeClear(int32 x, int32 y);

    // 评估士和象的协同防守
    int32 EvaluateShiXiangCooperation(int32 shiCount, int32 xiangCount, EChessColor color);

    // 评估防守结构
    int32 EvaluateDefensiveStructure(EChessColor color);

    // 评估走法后对对方的威胁
    int32 EvaluateThreatAfterMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess);

    bool IsBlockingAttack(const FChessMove2P& move, EChessColor color);

    // 检查走法是否保护了关键位置
    bool IsProtectingKeySquare(const FChessMove2P& move, EChessColor color);

    // 检查棋子是否能攻击指定位置
    bool CanAttackPosition(int32 fromX, int32 fromY, int32 toX, int32 toY, EChessColor attackerColor);

    // 检查走法是否保护将/帅
    bool IsMoveProtectingKing(const FChessMove2P& move, EChessColor color);

    // 检查棋子是否在防守位置
    bool IsInDefensivePosition(int32 x, int32 y, EChessColor color);

    // 计算棋子的防守贡献
    int32 CalculateDefensiveContribution(int32 x, int32 y, TWeakObjectPtr<AChesses> chess);

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

    // 专门处理被将军情况的搜索
    TWeakObjectPtr<AChesses> GetBestMoveWhenInCheck(FChessMove2P& bestMove, TArray<FChessMove2P>& allMoves);

    // 快速评估保将走法
    int32 QuickEvaluateEscapeMove(const FChessMove2P& move);

    // 评估走法安全性
    int32 EvaluateMoveSafety(const FChessMove2P& move);

    // 被将军时的特殊Minimax搜索，使用更激进的剪枝
    int32 MinimaxWhenInCheck(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer);

    // 快速评估函数，用于被将军时的快速评估
    int32 QuickEvaluate();

    // 获取最佳的几个保将走法
    TArray<FChessMove2P> GetTopEscapeMoves(const TArray<FChessMove2P>& escapeMoves, int32 count);
};
