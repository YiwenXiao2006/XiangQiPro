// Copyright 2026 Ultimate Player All Rights Reserved.


#include "AI2P.h"
#include "ChessBoard2P.h"
#include "../Chess/Chesses.h"
#include <Kismet/GameplayStatics.h>

UAI2P::UAI2P()
{
}

void UAI2P::SetDepth(int32 Depth)
{
    maxDepth = Depth;
    timeLimit = (Depth + 2) * 1000;
}

void UAI2P::SetBoard(TWeakObjectPtr<ChessBoard2P> newBoard)
{
    board2P = newBoard;
}

int32 UAI2P::Evaluate(EChessColor color)
{
    if (!board2P.IsValid()) return 0;

    EChessColor colorOtherSide = (color == EChessColor::BLACK ? EChessColor::RED : EChessColor::BLACK);
    int32 score = 0;

    // 基础棋子价值
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    // 检查是否被将军（非常重要！）
    bool blackInCheck = board2P->IsKingInCheck(color);
    bool redInCheck = board2P->IsKingInCheck(colorOtherSide);

    // 将军威胁评估（给被将军方很大的惩罚）
    if (blackInCheck) score -= 500;  // AI被将军，严重惩罚
    if (redInCheck) score += 300;    // 玩家被将军，适当奖励

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid())
            {
                int32 value = chessValues[(int32)chess->GetType()];

                if (chess->GetColor() == color)
                {
                    score += value;
                }
                else
                {
                    score -= value;
                }
            }
        }
    }

    // 添加局面评估因素
    score += EvaluateMobility(color) - EvaluateMobility(colorOtherSide);
    score += EvaluateThreats(color) - EvaluateThreats(colorOtherSide);
    score += EvaluateKingSafety(color) - EvaluateKingSafety(colorOtherSide);

    return score;
}

void UAI2P::InitializePositionScores(int32 positionScores[2][10][9])
{
    // 简化的位置价值表，需要根据实际象棋知识完善
    for (int32 color = 0; color < 2; color++)
    {
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                // 中心位置更有价值
                int32 centerBonus = 0;
                if (j >= 3 && j <= 5) centerBonus = 10;

                positionScores[color][i][j] = centerBonus;
            }
        }
    }
}

int32 UAI2P::EvaluateMobility(EChessColor color)
{
    // 评估行动力（可走棋步数量）
    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(color);
    return moves.Num() * 2; // 每个可走位置加2分
}

int32 UAI2P::EvaluateThreats(EChessColor color)
{
    // 评估威胁（攻击对方棋子的能力）
    int32 threatScore = 0;
    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    TArray<FChessMove2P> attacks = board2P->GenerateAllMoves(color);
    for (const FChessMove2P& attack : attacks)
    {
        TWeakObjectPtr<AChesses> target = board2P->GetChess(attack.to.X, attack.to.Y);
        if (target.IsValid() && target->GetColor() == opponentColor)
        {
            // 根据被攻击棋子的价值给予威胁分数
            int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };
            threatScore += chessValues[(int32)target->GetType()] / 20;
        }
    }

    return threatScore;
}

int32 UAI2P::EvaluateKingSafety(EChessColor color)
{
    if (!board2P.IsValid()) return 0;

    int32 safetyScore = 0;

    // 找到将/帅的位置
    int32 kingX = -1, kingY = -1;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid() && chess->GetColor() == color)
            {
                kingX = i;
                kingY = j;
                break;
            }
        }
        if (kingX != -1) break;
    }

    if (kingX == -1) return -1000; // 将/帅不存在，严重惩罚

    // 评估将/帅周围的保护
    int32 protectionCount = 0;
    int32 attackCount = 0;

    // 检查周围是否有己方棋子保护
    int32 directions[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {-1,1}, {1,-1}, {1,1} };
    for (int32 i = 0; i < 8; i++)
    {
        int32 x = kingX + directions[i][0];
        int32 y = kingY + directions[i][1];

        if (board2P->IsValidPosition(x, y))
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(x, y);
            if (chess.IsValid() && chess->GetColor() == color)
            {
                protectionCount++;
            }
        }
    }

    safetyScore = protectionCount * 10 - attackCount * 15;

    return safetyScore;
}

int32 UAI2P::EvaluateMove(const FChessMove2P& move)
{
    if (!board2P.IsValid()) return 0;

    int32 score = 0;
    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);
    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);

    // 1. 检查这个走法是否会导致自己被将军（非常重要！）
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    bool causesCheck = board2P->IsKingInCheck(EChessColor::BLACK);

    board2P->UndoTestMove(move, capturedChess);

    if (causesCheck)
    {
        // 这个走法会导致自己被将军，严重惩罚
        score -= 10000;
        return score; // 直接返回，不再评估其他因素
    }

    // 基础棋子价值
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    // 2. 吃子价值（MVV-LVA）
    if (targetChess.IsValid())
    {
        int32 victimValue = chessValues[(int32)targetChess->GetType()];
        int32 attackerValue = movingChess.IsValid() ? chessValues[(int32)movingChess->GetType()] : 0;

        score += victimValue * 10 - attackerValue;
    }

    // 3. 走法安全性：走法后是否处于安全位置
    score += EvaluateMoveSafety(move);

    // 4. 位置价值：根据棋子类型和位置给予奖励
    score += EvaluatePositionValue(move, movingChess);

    // 5. 威胁评估：这个走法是否威胁对方重要棋子
    score += EvaluateThreatAfterMove(move, movingChess);

    return score;
}

// 评估走法后的位置价值
int32 UAI2P::EvaluatePositionValue(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess) const
{
    if (!movingChess.IsValid()) return 0;

    int32 score = 0;
    EChessType type = movingChess->GetType();

    // 简化的位置价值表
    // 中心位置通常更有价值
    int32 centerBonus = 0;
    if (move.to.Y >= 3 && move.to.Y <= 5)
    {
        centerBonus = 5;
    }

    // 根据棋子类型给予不同的位置奖励
    switch (type)
    {
    case EChessType::MA:
        // 马在中心位置更有威力
        score += centerBonus * 2;
        break;

    case EChessType::PAO:
        // 炮在河界附近有更好的控制
        if (move.to.X == 4 || move.to.X == 5)
        {
            score += 10;
        }
        break;

    case EChessType::BING:
        // 兵过河后有奖励
        if ((movingChess->GetColor() == EChessColor::BLACK && move.to.X <= 4) ||
            (movingChess->GetColor() == EChessColor::RED && move.to.X >= 5))
        {
            score += 15;
        }
        break;

    default:
        score += centerBonus;
        break;
    }

    return score;
}

// 评估走法后对对方的威胁
int32 UAI2P::EvaluateThreatAfterMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess) const
{
    if (!board2P.IsValid() || !movingChess.IsValid()) return 0;

    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    int32 threatScore = 0;

    // 检查这个走法是否能威胁到对方的重要棋子
    EChessColor opponentColor = EChessColor::RED;

    // 查找对方的将/帅
    int32 opponentKingX = -1, opponentKingY = -1;
    EChessType opponentKingType = EChessType::JIANG;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == opponentKingType && chess->GetColor() == opponentColor)
            {
                opponentKingX = i;
                opponentKingY = j;
                break;
            }
        }
        if (opponentKingX != -1) break;
    }

    // 如果能直接攻击将/帅，给予高奖励
    if (opponentKingX != -1 && opponentKingY != -1)
    {
        if (CanAttackPosition(move.to.X, move.to.Y, opponentKingX, opponentKingY, EChessColor::BLACK))
        {
            threatScore += 50; // 将军威胁
        }
    }

    // 检查是否能攻击对方其他重要棋子
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> target = board2P->GetChess(i, j);
            if (target.IsValid() && target->GetColor() == opponentColor)
            {
                // 检查移动后的棋子是否能攻击这个目标
                if (CanAttackPosition(move.to.X, move.to.Y, i, j, EChessColor::BLACK))
                {
                    threatScore += chessValues[(int32)target->GetType()] / 20;
                }
            }
        }
    }

    board2P->UndoTestMove(move, capturedChess);

    return threatScore;
}

// 检查棋子是否能攻击指定位置
bool UAI2P::CanAttackPosition(int32 fromX, int32 fromY, int32 toX, int32 toY, EChessColor attackerColor) const
{
    if (!board2P.IsValid()) return false;

    // 这里可以调用棋盘类的相应方法，或者重新实现攻击判断逻辑
    // 由于棋盘类已经有GenerateMovesForChess，我们可以利用它
    TWeakObjectPtr<AChesses> attacker = board2P->GetChess(fromX, fromY);
    if (!attacker.IsValid() || attacker->GetColor() != attackerColor)
        return false;

    // 生成这个棋子的所有可能走法
    TArray<FChessMove2P> possibleMoves = board2P->GenerateMovesForChess(fromX, fromY, attacker);

    // 检查是否能走到目标位置
    for (const FChessMove2P& move : possibleMoves)
    {
        if (move.to.X == toX && move.to.Y == toY)
        {
            return true;
        }
    }

    return false;
}

// 检查走法是否保护将/帅
bool UAI2P::IsMoveProtectingKing(const FChessMove2P& move, EChessColor color) const
{
    if (!board2P.IsValid()) return false;

    // 找到将/帅的位置
    int32 kingX = -1, kingY = -1;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG && chess->GetColor() == color)
            {
                kingX = i;
                kingY = j;
                break;
            }
        }
        if (kingX != -1) break;
    }

    if (kingX == -1) return false;

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查走法后是否阻止了对将/帅的攻击
    EChessColor opponentColor = (color == EChessColor::RED) ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> opponentMoves = board2P->GenerateAllMoves(opponentColor);

    bool wasAttackingKing = false;
    for (const FChessMove2P& oppMove : opponentMoves)
    {
        if (oppMove.to.X == kingX && oppMove.to.Y == kingY)
        {
            wasAttackingKing = true;
            break;
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    // 重新检查原始局面
    TArray<FChessMove2P> originalOpponentMoves = board2P->GenerateAllMoves(opponentColor);
    bool isAttackingKing = false;
    for (const FChessMove2P& oppMove : originalOpponentMoves)
    {
        if (oppMove.to.X == kingX && oppMove.to.Y == kingY)
        {
            isAttackingKing = true;
            break;
        }
    }

    // 如果走法前将/帅被攻击，走法后不再被攻击，说明这个走法保护了将/帅
    return isAttackingKing && !wasAttackingKing;
}

bool UAI2P::IsTimeOut()
{
    return Timer.GetElapsedMilliseconds() > timeLimit;
}

int32 UAI2P::Minimax(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer)
{
    if (IsTimeOut())
    {
        return maximizingPlayer ? INT_MIN + 1000 : INT_MAX - 1000;
    }

    if (depth == 0)
    {
        return QuiescenceSearch(alpha, beta, maximizingPlayer);
    }

    EChessColor currentColor = maximizingPlayer ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(currentColor);

    if (moves.IsEmpty())
    {
        bool inCheck = board2P->IsKingInCheck(currentColor);
        if (inCheck)
        {
            return maximizingPlayer ? INT_MIN + depth * 100 : INT_MAX - depth * 100;
        }
        return 0; // 僵局
    }

    // 走法排序
    SortMovesWithHistory(moves, depth);

    if (maximizingPlayer)
    {
        int32 maxEval = INT_MIN;
        bool hasValidMove = false;

        for (const FChessMove2P& move : moves)
        {
            if (IsTimeOut()) break;

            // 检查这个走法是否会导致自己被将军
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            bool causesCheck = board2P->IsKingInCheck(EChessColor::BLACK);

            int32 eval;
            if (causesCheck)
            {
                // 这个走法会导致被将军，给予严重惩罚
                eval = INT_MIN + 500; // 比被将死稍好一点
            }
            else
            {
                eval = Minimax(depth - 1, alpha, beta, false);
                hasValidMove = true;
            }

            board2P->UndoTestMove(move, capturedChess);

            if (eval > maxEval)
            {
                maxEval = eval;
            }

            alpha = FMath::Max(alpha, eval);
            if (beta <= alpha)
            {
                break;
            }
        }

        // 如果没有有效的走法（所有走法都会导致被将军），返回被将军的惩罚
        if (!hasValidMove && maxEval < INT_MIN + 1000)
        {
            return INT_MIN + 200; // 所有走法都会导致被将军
        }

        return maxEval;
    }
    else
    {
        int32 minEval = INT_MAX;
        bool hasValidMove = false;

        for (const FChessMove2P& move : moves)
        {
            if (IsTimeOut()) break;

            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            bool causesCheck = board2P->IsKingInCheck(EChessColor::RED);

            int32 eval;
            if (causesCheck)
            {
                // 这个走法会导致被将军，给予严重惩罚（对红方来说是好事）
                eval = INT_MAX - 500;
            }
            else
            {
                eval = Minimax(depth - 1, alpha, beta, true);
                hasValidMove = true;
            }

            board2P->UndoTestMove(move, capturedChess);

            if (eval < minEval)
            {
                minEval = eval;
            }

            beta = FMath::Min(beta, eval);
            if (beta <= alpha)
            {
                break;
            }
        }

        if (!hasValidMove && minEval > INT_MAX - 1000)
        {
            return INT_MAX - 200;
        }

        return minEval;
    }
}

void UAI2P::SortMoves(TArray<FChessMove2P>& moves)
{
    // 使用lambda表达式进行排序
    moves.Sort([this](const FChessMove2P& moveA, const FChessMove2P& moveB) {
        return EvaluateMove(moveA) > EvaluateMove(moveB);
        });
}

int32 UAI2P::QuiescenceSearch(int32 alpha, int32 beta, bool maximizingPlayer)
{
    int32 stand_pat = Evaluate();

    if (maximizingPlayer)
    {
        if (stand_pat >= beta)
        {
            return beta;
        }
        if (stand_pat > alpha)
        {
            alpha = stand_pat;
        }
    }
    else
    {
        if (stand_pat <= alpha)
        {
            return alpha;
        }
        if (stand_pat < beta)
        {
            beta = stand_pat;
        }
    }

    // 只生成吃子走法
    EChessColor currentColor = maximizingPlayer ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> captureMoves = GenerateCaptureMoves(currentColor);

    SortMoves(captureMoves);

    if (maximizingPlayer)
    {
        for (const FChessMove2P& move : captureMoves)
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            if (!capturedChess.IsValid())
            {
                continue;
            }

            board2P->MakeTestMove(move);
            int32 score = QuiescenceSearch(alpha, beta, false);
            board2P->UndoTestMove(move, capturedChess);

            if (score >= beta)
            {
                return beta;
            }
            if (score > alpha) 
            {
                alpha = score;
            }
        }
        return alpha;
    }
    else
    {
        for (const FChessMove2P& move : captureMoves)
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            if (!capturedChess.IsValid()) continue;

            board2P->MakeTestMove(move);
            int32 score = QuiescenceSearch(alpha, beta, true);
            board2P->UndoTestMove(move, capturedChess);

            if (score <= alpha)
            {
                return alpha;
            }
            if (score < beta)
            {
                beta = score;
            }
        }
        return beta;
    }
}

TArray<FChessMove2P> UAI2P::GenerateCaptureMoves(EChessColor color)
{
    TArray<FChessMove2P> allMoves = board2P->GenerateAllMoves(color);
    TArray<FChessMove2P> captureMoves;

    for (const FChessMove2P& move : allMoves)
    {
        TWeakObjectPtr<AChesses> target = board2P->GetChess(move.to.X, move.to.Y);
        if (target.IsValid() && target->GetColor() != color)
        {
            captureMoves.Add(move);
        }
    }

    return captureMoves;
}

void UAI2P::SortMovesWithHistory(TArray<FChessMove2P>& moves, int32 depth)
{
    moves.Sort([this, depth](const FChessMove2P& moveA, const FChessMove2P& moveB) {
        int32 scoreA = EvaluateMove(moveA) + historyTable[(int32)moveA.from.X][(int32)moveA.from.Y][(int32)moveA.to.X][(int32)moveA.to.Y] * depth;
        int32 scoreB = EvaluateMove(moveB) + historyTable[(int32)moveB.from.X][(int32)moveB.from.Y][(int32)moveB.to.X][(int32)moveB.to.Y] * depth;
        return scoreA > scoreB;
        });
}

void UAI2P::UpdateHistoryTable(const FChessMove2P& move, int32 depth)
{
    // 好的走法在历史表中增加分数
    historyTable[(int32)move.from.X][(int32)move.from.Y][(int32)move.to.X][(int32)move.to.Y] += depth * depth;

    // 防止历史表值过大
    if (historyTable[(int32)move.from.X][(int32)move.from.Y][(int32)move.to.X][(int32)move.to.Y] > 1000000)
    {
        // 定期衰减历史表值
        for (int32 i = 0; i < 10; i++)
            for (int32 j = 0; j < 9; j++)
                for (int32 k = 0; k < 10; k++)
                    for (int32 l = 0; l < 9; l++)
                        historyTable[i][j][k][l] /= 2;
    }
}

TWeakObjectPtr<AChesses> UAI2P::GetBestMove(FChessMove2P& bestMove)
{
    if (!board2P.IsValid()) return nullptr;

    Timer.Start();

    // 首先检查当前是否被将军
    bool inCheck = board2P->IsKingInCheck(EChessColor::BLACK);

    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(EChessColor::BLACK);
    TWeakObjectPtr<AChesses> movedChess;

    // 如果被将军，使用专门的保将搜索
    if (inCheck)
    {
        ULogger::Log(TEXT("AI is in check! Prioritizing king safety moves."));
        return GetBestMoveWhenInCheck(bestMove, moves);
    }

    // 正常情况下的迭代加深搜索
    FChessMove2P currentBestMove;
    int32 currentBestValue = INT_MIN;

    // 首先快速筛选出不会导致被将军的走法
    TArray<FChessMove2P> safeMoves;
    for (const FChessMove2P& move : moves)
    {
        TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
        board2P->MakeTestMove(move);

        bool causesCheck = board2P->IsKingInCheck(EChessColor::BLACK);

        board2P->UndoTestMove(move, capturedChess);

        if (!causesCheck)
        {
            safeMoves.Add(move);
        }
    }

    // 如果没有安全的走法，记录警告但继续搜索所有走法
    if (safeMoves.IsEmpty())
    {
        ULogger::LogWarning(TEXT("No safe moves found! AI may move into check."));
        safeMoves = moves; // 使用所有走法，包括不安全的
    }
    else
    {
        moves = safeMoves; // 使用安全走法
        ULogger::Log(FString::Printf(TEXT("Filtered to %d safe moves"), safeMoves.Num()));
    }

    for (int32 depth = 1; depth <= maxDepth; depth++)
    {
        if (IsTimeOut()) break;

        int32 bestValueThisIteration = INT_MIN;
        FChessMove2P bestMoveThisIteration;
        TWeakObjectPtr<AChesses> bestChessThisIteration;

        SortMovesWithHistory(moves, depth);

        for (const FChessMove2P& move : moves)
        {
            if (IsTimeOut()) break;

            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            // 快速检查这个走法是否安全
            bool causesCheck = board2P->IsKingInCheck(EChessColor::BLACK);
            int32 moveValue;

            if (causesCheck)
            {
                // 导致被将军，严重惩罚
                moveValue = INT_MIN + 1000;
            }
            else
            {
                moveValue = Minimax(depth - 1, INT_MIN, INT_MAX, false);
            }

            board2P->UndoTestMove(move, capturedChess);

            if (moveValue > bestValueThisIteration)
            {
                bestChessThisIteration = board2P->GetChess(move.from.X, move.from.Y);
                bestValueThisIteration = moveValue;
                bestMoveThisIteration = move;
            }
        }

        if (bestValueThisIteration > INT_MIN)
        {
            currentBestValue = bestValueThisIteration;
            currentBestMove = bestMoveThisIteration;
            movedChess = bestChessThisIteration;
            UpdateHistoryTable(currentBestMove, depth);

            // 检查最佳走法是否安全
            TWeakObjectPtr<AChesses> testCaptured = board2P->GetChess(currentBestMove.to.X, currentBestMove.to.Y);
            board2P->MakeTestMove(currentBestMove);
            bool isSafe = !board2P->IsKingInCheck(EChessColor::BLACK);
            board2P->UndoTestMove(currentBestMove, testCaptured);

            if (isSafe)
            {
                ULogger::Log(FString::Printf(TEXT("Depth %d: Found safe move with value %d"), depth, currentBestValue));
            }
            else
            {
                ULogger::LogWarning(FString::Printf(TEXT("Depth %d: Best move causes check! Value: %d"), depth, currentBestValue));
            }
        }
    }

    bestMove = currentBestMove;

    return movedChess;
}

// 专门处理被将军情况的搜索
TWeakObjectPtr<AChesses> UAI2P::GetBestMoveWhenInCheck(FChessMove2P& bestMove, TArray<FChessMove2P>& allMoves)
{
    // 第一步：筛选出所有能够解除将军的走法
    TArray<FChessMove2P> escapeMoves;

    for (const FChessMove2P& move : allMoves)
    {
        TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
        board2P->MakeTestMove(move);

        bool stillInCheck = board2P->IsKingInCheck(EChessColor::BLACK);

        board2P->UndoTestMove(move, capturedChess);

        if (!stillInCheck)
        {
            escapeMoves.Add(move);
        }
    }

    // 如果没有走法可以解除将军，说明被将死
    if (escapeMoves.IsEmpty())
    {
        ULogger::LogWarning(TEXT("UAI2P::GetBestMoveWhenInCheck: Checkmate! AI has no escape moves."));
        // 返回第一个走法（虽然都会被将死，但总得走一步）
        if (!allMoves.IsEmpty())
        {
            bestMove = allMoves[0];
            return board2P->GetChess(bestMove.from.X, bestMove.from.Y);
        }
        return nullptr;
    }

    ULogger::Log(FString::Printf(TEXT("UAI2P::GetBestMoveWhenInCheck: Found %d escape moves from check"), escapeMoves.Num()));

    // 第二步：优先评估这些保将走法，使用更简单但更快的评估
    FChessMove2P bestEscapeMove;
    int32 bestEscapeValue = INT_MIN;
    TWeakObjectPtr<AChesses> bestEscapeChess;

    // 快速评估：主要看吃子价值和位置安全
    for (const FChessMove2P& move : escapeMoves)
    {
        if (IsTimeOut()) break;

        int32 moveValue = QuickEvaluateEscapeMove(move);

        if (moveValue > bestEscapeValue)
        {
            bestEscapeValue = moveValue;
            bestEscapeMove = move;
            bestEscapeChess = board2P->GetChess(move.from.X, move.from.Y);
        }
    }

    // 第三步：如果还有时间，对最佳保将走法进行更深层的搜索
    if (!IsTimeOut())
    {
        // 只对最佳的几个保将走法进行深入搜索
        TArray<FChessMove2P> topEscapeMoves = GetTopEscapeMoves(escapeMoves, 3); // 取前3个

        for (const FChessMove2P& move : topEscapeMoves)
        {
            if (IsTimeOut()) break;

            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            // 被将军时搜索更深，但使用更窄的搜索窗口提高速度
            int32 moveValue = MinimaxWhenInCheck(2, bestEscapeValue, INT_MAX, false);

            board2P->UndoTestMove(move, capturedChess);

            if (moveValue > bestEscapeValue)
            {
                bestEscapeValue = moveValue;
                bestEscapeMove = move;
                bestEscapeChess = board2P->GetChess(move.from.X, move.from.Y);
            }
        }
    }

    bestMove = bestEscapeMove;

    ULogger::Log(FString::Printf(TEXT("Selected escape move with value %d"), bestEscapeValue));

    return bestEscapeChess;
}

// 快速评估保将走法
int32 UAI2P::QuickEvaluateEscapeMove(const FChessMove2P& move)
{
    if (!board2P.IsValid()) return 0;

    int32 score = 0;
    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);
    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);

    // 基础棋子价值
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    // 1. 吃子价值（最重要）
    if (targetChess.IsValid())
    {
        score += chessValues[(int32)targetChess->GetType()] / 10;
    }

    // 2. 走法安全性：走法后是否处于安全位置
    score += EvaluateMoveSafety(move);

    // 3. 将/帅移动的奖励（将/帅逃离危险位置）
    if (movingChess.IsValid() && movingChess->GetType() == EChessType::JIANG)
    {
        score += 50; // 将/帅移动的额外奖励
    }

    // 4. 保护性走法：走到可以保护将/帅的位置
    if (IsMoveProtectingKing(move, EChessColor::BLACK))
    {
        score += 30;
    }

    return score;
}

// 评估走法安全性
int32 UAI2P::EvaluateMoveSafety(const FChessMove2P& move)
{
    if (!board2P.IsValid()) return 0;

    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);

    board2P->MakeTestMove(move);

    int32 safetyScore = 0;

    // 检查移动后的棋子是否会被立即吃掉
    EChessColor opponentColor = EChessColor::RED;
    TArray<FChessMove2P> opponentMoves = board2P->GenerateAllMoves(opponentColor);

    bool canBeCaptured = false;
    for (const FChessMove2P& oppMove : opponentMoves)
    {
        if (oppMove.to.X == move.to.X && oppMove.to.Y == move.to.Y)
        {
            canBeCaptured = true;

            // 评估反击价值：如果我们被吃，能吃什么
            TWeakObjectPtr<AChesses> attacker = board2P->GetChess(oppMove.from.X, oppMove.from.Y);
            if (attacker.IsValid() && movingChess.IsValid())
            {
                int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };
                int32 attackerValue = chessValues[(int32)attacker->GetType()];
                int32 defenderValue = chessValues[(int32)movingChess->GetType()];

                // 如果我们可以吃回更有价值的棋子，安全性较高
                if (defenderValue < attackerValue)
                {
                    safetyScore -= 20; // 不利交换
                }
                else if (defenderValue > attackerValue)
                {
                    safetyScore += 15; // 有利交换
                }
            }
            break;
        }
    }

    if (!canBeCaptured)
    {
        safetyScore += 25; // 安全位置奖励
    }

    board2P->UndoTestMove(move, capturedChess);

    return safetyScore;
}

// 被将军时的特殊Minimax搜索，使用更激进的剪枝
int32 UAI2P::MinimaxWhenInCheck(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer)
{
    if (IsTimeOut()) return maximizingPlayer ? INT_MIN : INT_MAX;

    // 被将军时，我们主要关注解除威胁，所以评估函数更简单
    if (depth == 0)
    {
        return QuickEvaluate();
    }

    EChessColor currentColor = maximizingPlayer ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(currentColor);

    if (moves.IsEmpty())
    {
        bool inCheck = board2P->IsKingInCheck(currentColor);
        if (inCheck)
        {
            return maximizingPlayer ? INT_MIN + depth : INT_MAX - depth;
        }
        return 0; // 僵局
    }

    // 被将军时使用更简单的排序
    TArray<FChessMove2P> sortedMoves = moves;
    sortedMoves.Sort([this](const FChessMove2P& a, const FChessMove2P& b) {
        return QuickEvaluateEscapeMove(a) > QuickEvaluateEscapeMove(b);
        });

    // 限制搜索的走法数量以提高速度
    int32 moveLimit = FMath::Min(5, sortedMoves.Num()); // 只搜索前5个走法

    if (maximizingPlayer)
    {
        int32 maxEval = INT_MIN;
        for (int32 i = 0; i < moveLimit; i++)
        {
            if (IsTimeOut()) break;

            const FChessMove2P& move = sortedMoves[i];
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            int32 eval = MinimaxWhenInCheck(depth - 1, alpha, beta, false);

            board2P->UndoTestMove(move, capturedChess);

            if (eval > maxEval) maxEval = eval;
            alpha = FMath::Max(alpha, eval);

            if (beta <= alpha) break;
        }
        return maxEval;
    }
    else
    {
        int32 minEval = INT_MAX;
        for (int32 i = 0; i < moveLimit; i++)
        {
            if (IsTimeOut()) break;

            const FChessMove2P& move = sortedMoves[i];
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            int32 eval = MinimaxWhenInCheck(depth - 1, alpha, beta, true);

            board2P->UndoTestMove(move, capturedChess);

            if (eval < minEval) minEval = eval;
            beta = FMath::Min(beta, eval);

            if (beta <= alpha) break;
        }
        return minEval;
    }
}

// 快速评估函数，用于被将军时的快速评估
int32 UAI2P::QuickEvaluate()
{
    if (!board2P.IsValid()) return 0;

    int32 score = 0;
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    // 被将军时的简化评估，主要看棋子价值和将军状态
    bool blackInCheck = board2P->IsKingInCheck(EChessColor::BLACK);
    bool redInCheck = board2P->IsKingInCheck(EChessColor::RED);

    if (blackInCheck) score -= 200;
    if (redInCheck) score += 100;

    // 简化的棋子价值计算
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid())
            {
                int32 value = chessValues[(int32)chess->GetType()] / 20; // 降低权重，加快计算

                if (chess->GetColor() == EChessColor::BLACK)
                {
                    score += value;
                }
                else
                {
                    score -= value;
                }
            }
        }
    }

    return score;
}

// 获取最佳的几个保将走法
TArray<FChessMove2P> UAI2P::GetTopEscapeMoves(const TArray<FChessMove2P>& escapeMoves, int32 count)
{
    TArray<TPair<int32, FChessMove2P>> scoredMoves;

    for (const FChessMove2P& move : escapeMoves)
    {
        int32 score = QuickEvaluateEscapeMove(move);
        scoredMoves.Add(TPair<int32, FChessMove2P>(score, move));
    }

    // 按分数降序排序
    scoredMoves.Sort([](const TPair<int32, FChessMove2P>& a, const TPair<int32, FChessMove2P>& b) {
        return a.Key > b.Key;
        });

    TArray<FChessMove2P> topMoves;
    for (int32 i = 0; i < FMath::Min(count, scoredMoves.Num()); i++)
    {
        topMoves.Add(scoredMoves[i].Value);
    }

    return topMoves;
}
