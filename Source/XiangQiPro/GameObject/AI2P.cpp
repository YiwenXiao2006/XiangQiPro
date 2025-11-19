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
}

void UAI2P::SetBoard(TWeakObjectPtr<ChessBoard2P> newBoard)
{
    board2P = newBoard;
}

int32 UAI2P::Evaluate()
{
    if (!board2P.IsValid())
    {
        ULogger::LogError(TEXT("Can't evaluate score, because chess board is nullptr!"));
        return 0;
    }

    int32 score = 0;

    // 棋子价值
    int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };

    for (int32 i = 0; i < 10; i++) 
    {
        for (int32 j = 0; j < 9; j++) 
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess != nullptr) 
            {
                int32 value = chessValues[(int32) chess->GetType()];
                if (chess->GetColor() == EChessColor::BLACK) // AI是黑方
                {
                    score += value;
                }
                else // 玩家是红方
                {
                    score -= value;
                }
            }
        }
    }

    return score;
}

bool UAI2P::IsTimeOut()
{
    return Timer.GetElapsedMilliseconds() > timeLimit;
}

int32 UAI2P::Minimax(int32 depth, int32 alpha, int32 beta, bool maximizingPlayer)
{
    if (depth == 0) 
    {
        return Evaluate();
    }

    EChessColor currentColor = maximizingPlayer ? EChessColor::BLACK : EChessColor::RED;
    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(currentColor);

    if (moves.IsEmpty()) 
    {
        if (maximizingPlayer) 
        {
            return INT_MIN + 1;  // 黑方被将死
        }
        else 
        {
            return INT_MAX - 1;  // 红方被将死
        }
    }

    if (maximizingPlayer) 
    {
        int32 maxEval = INT_MIN;
        for (FChessMove2P move : moves) 
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);
            int32 eval = Minimax(depth - 1, alpha, beta, false);
            board2P->UndoTestMove(move, capturedChess);

            maxEval = Math::Max(maxEval, eval);
            alpha = Math::Max(alpha, eval);
            if (beta <= alpha) 
            {
                break;  // Beta剪枝
            }
        }
        return maxEval;
    }
    else 
    {
        int32 minEval = INT_MAX;
        for (FChessMove2P move : moves) 
        {
            TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
            board2P->MakeTestMove(move);
            int32 eval = Minimax(depth - 1, alpha, beta, true);
            board2P->UndoTestMove(move, capturedChess);

            minEval = Math::Min(minEval, eval);
            beta = Math::Min(beta, eval);
            if (beta <= alpha) 
            {
                break;  // Alpha剪枝
            }
        }
        return minEval;
    }
}

void UAI2P::SortMoves(TArray<FChessMove2P>& moves)
{
}

TWeakObjectPtr<AChesses> UAI2P::GetBestMove(FChessMove2P& bestMove)
{
    if (!board2P.IsValid())
    {
        ULogger::LogError(TEXT("UAI2P::GetBestMove: Can't get best movement, because board is nullptr!"));
        return nullptr;
    }

    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(EChessColor::BLACK);
    TWeakObjectPtr<AChesses> movedChess;
    int32 bestValue = INT_MIN;

    for (FChessMove2P move : moves) 
    {
        TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
        board2P->MakeTestMove(move);
        int32 moveValue = Minimax(maxDepth - 1, INT_MIN, INT_MAX, false);
        board2P->UndoTestMove(move, capturedChess);

        if (moveValue > bestValue) 
        {
            movedChess = board2P->GetChess(move.from.X, move.from.Y);
            bestValue = moveValue;
            bestMove = move;
        }
    }
    
    return movedChess;
}
