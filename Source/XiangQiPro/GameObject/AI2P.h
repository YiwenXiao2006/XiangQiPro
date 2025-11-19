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
public:

    UAI2P();

    // 设置搜索深度
    void SetDepth(int32 Depth);

    // 设置棋盘状态
    void SetBoard(TWeakObjectPtr<UChessBoard2P> newBoard);

    // 评估函数 - 简单实现，只考虑棋子价值
    int32 Evaluate();

    // 检查是否超时
    bool IsTimeOut();

    // 极小化极大算法
    int32 Minimax(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer);

    // 走法排序函数
    void SortMoves(TArray<FChessMove2P>& moves);

    // 获取AI的最佳走法
    TWeakObjectPtr<AChesses> GetBestMove(FChessMove2P& bestMove);
};
