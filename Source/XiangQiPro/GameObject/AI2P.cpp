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

int32 UAI2P::Evaluate()
{
    if (!board2P.IsValid()) return 0;

    int32 score = 0;

    // 基础棋子价值
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    // 位置价值表（示例，需要根据象棋知识完善）
    int32 positionScores[2][10][9]; // [颜色][x][y]
    InitializePositionScores(positionScores);

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid())
            {
                int32 value = chessValues[(int32)chess->GetType()];

                // 添加位置价值
                if (chess->GetColor() == EChessColor::BLACK)
                {
                    value += positionScores[0][i][j];
                    score += value;
                }
                else
                {
                    value += positionScores[1][i][j];
                    score -= value;
                }
            }
        }
    }

    // 添加局面评估因素
    score += EvaluateMobility(EChessColor::BLACK) - EvaluateMobility(EChessColor::RED);
    score += EvaluateThreats(EChessColor::BLACK) - EvaluateThreats(EChessColor::RED);

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

int32 UAI2P::EvaluateMove(const FChessMove2P& move) const
{
    if (!board2P.IsValid()) return 0;

    int32 score = 0;
    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);

    // 吃子走法优先（MVV-LVA：最价值受害者-最廉价攻击者）
    if (targetChess.IsValid())
    {
        int32 victimValue = 0;
        int32 attackerValue = 0;

        // 棋子价值表（与Evaluate函数一致）
        int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

        victimValue = chessValues[(int32)targetChess->GetType()];
        TWeakObjectPtr<AChesses> attacker = board2P->GetChess(move.from.X, move.from.Y);
        if (attacker.IsValid())
        {
            attackerValue = chessValues[(int32)attacker->GetType()];
        }

        // MVV-LVA：受害者价值越高，攻击者价值越低，分数越高
        score = victimValue * 10 - attackerValue;
    }

    return score;
}

bool UAI2P::IsTimeOut()
{
    return Timer.GetElapsedMilliseconds() > timeLimit;
}

int32 UAI2P::Minimax(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer)
{
    if (IsTimeOut()) return 0;
    if (depth == 0) return QuiescenceSearch(alpha, beta, maximizingPlayer);

    EChessColor currentColor = maximizingPlayer ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(currentColor);

    if (moves.IsEmpty())
    {
        return maximizingPlayer ? INT_MIN + depth : INT_MAX - depth;
    }

    // 使用更智能的排序
    TArray<FChessMove2P> sortedMoves = moves;
    SortMovesWithHistory(sortedMoves, depth);

    if (maximizingPlayer)
    {
        int32 maxEval = INT_MIN;
        for (const FChessMove2P& move : sortedMoves)
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            int32 eval = Minimax(depth - 1, alpha, beta, false);

            board2P->UndoTestMove(move, capturedChess);

            if (eval > maxEval) maxEval = eval;
            alpha = Math::Max(alpha, eval);

            if (beta <= alpha) break;
        }
        return maxEval;
    }
    else
    {
        int32 minEval = INT_MAX;
        for (const FChessMove2P& move : sortedMoves)
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            int32 eval = Minimax(depth - 1, alpha, beta, true);

            board2P->UndoTestMove(move, capturedChess);

            if (eval < minEval) minEval = eval;
            beta = Math::Min(beta, eval);

            if (beta <= alpha) break;
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

    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(EChessColor::BLACK);
    TWeakObjectPtr<AChesses> movedChess;

    // 迭代加深搜索
    FChessMove2P currentBestMove;
    int32 currentBestValue = INT_MIN;

    for (int32 depth = 1; depth <= maxDepth; depth++)
    {
        if (IsTimeOut()) break;

        int32 bestValueThisIteration = INT_MIN;
        FChessMove2P bestMoveThisIteration;
        TWeakObjectPtr<AChesses> bestChessThisIteration;

        // 使用历史表排序走法
        SortMovesWithHistory(moves, depth);

        for (const FChessMove2P& move : moves)
        {
            if (IsTimeOut()) break;

            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);

            int32 moveValue = Minimax(depth - 1, INT_MIN, INT_MAX, false);

            board2P->UndoTestMove(move, capturedChess);

            if (moveValue > bestValueThisIteration)
            {
                bestChessThisIteration = board2P->GetChess(move.from.X, move.from.Y);
                bestValueThisIteration = moveValue;
                bestMoveThisIteration = move;
            }
        }

        // 只有在找到有效走法时才更新最佳走法
        if (bestValueThisIteration > INT_MIN)
        {
            currentBestValue = bestValueThisIteration;
            currentBestMove = bestMoveThisIteration;
            movedChess = bestChessThisIteration;

            // 记录到历史表
            UpdateHistoryTable(currentBestMove, depth);
        }
    }

    bestMove = currentBestMove;

    return movedChess;
}
