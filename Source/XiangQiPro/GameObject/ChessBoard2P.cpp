// Copyright 2026 Ultimate Player All Rights Reserved.

#include "ChessBoard2P.h"
#include "ChessBoard2PActor.h"
#include "../Chess/Chesses.h"
#include "../GameObject/SettingPoint.h"


UChessBoard2P::UChessBoard2P()
{
}

void UChessBoard2P::InitializeBoard(TWeakObjectPtr<AChessBoard2PActor> ChessBoard2PActor)
{
    if (!ChessBoard2PActor.IsValid())
    {
        ULogger::LogError(TEXT("Can't initialize board data, because ChessBoard2PActor is nullptr!"));
        return;
    }

    // 清空棋盘
    for (int32 i = 0; i < 10; i++)
    {
        TArray<TWeakObjectPtr<AChesses>> ChessList = {};
        for (int32 j = 0; j < 9; j++)
        {
            ChessList.Add(nullptr);
        }
        Board.Add(ChessList);
    }
    for (int32 i = 0; i < 10; i++)
    {
        TArray<TWeakObjectPtr<ASettingPoint>> PointList = {};
        for (int32 j = 0; j < 9; j++)
        {
            PointList.Add(nullptr);
        }
        SettingPoints.Add(PointList);
    }

    // 从Actor处获取边界坐标
    FVector BorderLoc1 = ChessBoard2PActor->BorderLoc1;
    FVector BorderLoc2 = ChessBoard2PActor->BorderLoc2;
    
    // 计算间隔长度
    float LengthX = (BorderLoc2.X - BorderLoc1.X) / 9.f;
    float LengthY = (BorderLoc2.Y - BorderLoc1.Y) / 8.f;

    for (int32 i = 0; i < 10; i++)
    {
        TArray<FVector> LocList = {};
        for (int32 j = 0; j < 9; j++)
        {
            LocList.Add(FVector(BorderLoc1.X + LengthX * i - 0.02 * i, BorderLoc1.Y + LengthY * j, 81.3f));
        }
        BoardLocs.Add(LocList);
    }
}

void UChessBoard2P::ShowSettingPoint2P(TArray<FChessMove2P> Moves, TWeakObjectPtr<AChesses> Target)
{
    for (FChessMove2P move : Moves)
    {
        if (IsValidPosition(move.to.X, move.to.Y))
        {
            SettingPoints[move.to.X][move.to.Y]->SetActivate(true);
            SettingPoints[move.to.X][move.to.Y]->SetTargetChess(Target);
        }
        else
        {
            ULogger::LogWarning(FString("UChessBoard2P::ShowSettingPoint2P: Movement is invalid, move is :").Append(FString::FromInt(move.to.X)).Append(",").Append(FString::FromInt(move.to.Y)));
        }
    }
}

void UChessBoard2P::DismissSettingPoint2P()
{
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            SettingPoints[i][j]->SetActivate(false);
            SettingPoints[i][j]->SetTargetChess(nullptr);
        }
    }
}

void UChessBoard2P::SetSideToMove(EChessColor color)
{
    sideToMove = color;
}

TWeakObjectPtr<AChesses> UChessBoard2P::GetChess(int32 x, int32 y) const
{
    if (x >= 0 && x < 10 && y >= 0 && y < 9)
    {
        return Board[x][y];
    }
    return nullptr;
}

void UChessBoard2P::SetChess(int32 x, int32 y, TWeakObjectPtr<AChesses> Chess)
{
    if (x >= 0 && x < 10 && y >= 0 && y < 9) 
    {
        Board[x][y] = Chess;
    }
}

void UChessBoard2P::ApplyMove(TWeakObjectPtr<AChesses> target, FChessMove2P move)
{
    if (!target.IsValid())
    {
        ULogger::LogError(TEXT("UChessBoard2P::ApplyMove: Can't apply movement, because target chess is nullptr!"));
        return;
    }

    TWeakObjectPtr<AChesses> CaptureChess = GetChess(move.to.X, move.to.Y); // 尝试获取原有位置的棋子
    if (CaptureChess.IsValid()) // 存在棋子
    {
        CaptureChess->Defeated(); // 棋子被吃掉
    }

    SetChess(move.to.X, move.to.Y, target);
    SetChess(move.from.X, move.from.Y, nullptr);
    target->ApplyMove(move);
}

void UChessBoard2P::MakeTestMove(const FChessMove2P& move)
{
    TWeakObjectPtr<AChesses> chess = GetChess(move.from.X, move.from.Y);

    SetChess(move.to.X, move.to.Y, chess);
    SetChess(move.from.X, move.from.Y, nullptr);
}

void UChessBoard2P::UndoTestMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> capturedPiece)
{
    TWeakObjectPtr<AChesses> chess = GetChess(move.to.X, move.to.Y);

    SetChess(move.from.X, move.from.Y, chess);
    SetChess(move.to.X, move.to.Y, capturedPiece);
}

bool UChessBoard2P::IsValidPosition(int32 x, int32 y) const
{
    return x >= 0 && x < 10 && y >= 0 && y < 9;
}

bool UChessBoard2P::IsInPalace(int32 x, int32 y, EChessColor color) const
{
    if (color == EChessColor::RED) 
    {
        return x >= 0 && x <= 2 && y >= 3 && y <= 5;
    }
    else 
    {
        return x >= 7 && x <= 9 && y >= 3 && y <= 5;
    }
}

int32 UChessBoard2P::CountPiecesBetween(int32 fromX, int32 fromY, int32 toX, int32 toY) const
{
    int32 count = 0;

    if (fromX == toX) // 同一行
    {
        int32 minY = Math::Min(fromY, toY);
        int32 maxY = Math::Max(fromY, toY);
        for (int32 y = minY + 1; y < maxY; y++) 
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(fromX, y);
            if (!Chess.IsValid())
            {
                continue;
            }

            if (Chess->GetType() != EChessType::EMPTY)
            {
                count++;
            }
        }
    }
    else if (fromY == toY) // 同一列
    { 
        int32 minX = Math::Min(fromX, toX);
        int32 maxX = Math::Max(fromX, toX);
        for (int32 x = minX + 1; x < maxX; x++) {
            TWeakObjectPtr<AChesses> Chess = GetChess(x, fromY);
            if (!Chess.IsValid())
            {
                continue;
            }
            if (Chess->GetType() != EChessType::EMPTY) {
                count++;
            }
        }
    }

    return count;
}

TArray<FChessMove2P> UChessBoard2P::GenerateAllMoves(EChessColor color)
{
    TArray<FChessMove2P> moves;

    for (int32 i = 0; i < 10; i++) 
    {
        for (int32 j = 0; j < 9; j++) 
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid())
            {
                continue;
            }
            if (Chess->GetType() != EChessType::EMPTY && Chess->GetColor() == color)
            {
                TArray<FChessMove2P> chessMoves = GenerateMovesForChess(i, j, Chess);
                moves.Append(chessMoves);
            }
        }
    }

    return moves;
}

TArray<FChessMove2P> UChessBoard2P::GenerateMovesForChess(int32 x, int32 y, TWeakObjectPtr<AChesses> chess)
{
    if (!chess.IsValid())
    {
        ULogger::LogError(TEXT("Can't generate moves for chess, because chess is nullptr!"));
        return TArray<FChessMove2P>();
    }
    TArray<FChessMove2P> moves;
    EChessType type = chess->GetType();
    EChessColor color = chess->GetColor();

    switch (type) 
    {
    case EChessType::JIANG:
        GenerateJiangMoves(x, y, color, moves);
        break;
    case EChessType::SHI:
        GenerateShiMoves(x, y, color, moves);
        break;
    case EChessType::XIANG:
        GenerateXiangMoves(x, y, color, moves);
        break;
    case EChessType::MA:
        GenerateMaMoves(x, y, color, moves);
        break;
    case EChessType::JV:
        GenerateJvMoves(x, y, color, moves);
        break;
    case EChessType::PAO:
        GeneratePaoMoves(x, y, color, moves);
        break;
    case EChessType::BING:
        GenerateBingMoves(x, y, color, moves);
        break;
    default:
        break;
    }

    return moves;
}

void UChessBoard2P::GenerateJiangMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 将/帅的移动方向：上、下、左、右
    int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

    for (int32 i = 0; i < 4; i++) 
    {
        int32 newX = x + directions[i][0];
        int32 newY = y + directions[i][1];

        // 检查是否在九宫格内
        if (IsInPalace(newX, newY, color))
        {
            TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
            if (target == nullptr || target->GetColor() != color)
            {
                moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
            }
        }
    }

    // 检查将帅相对
    if (color == EChessColor::RED) 
    {
        for (int32 i = x - 1; i >= 0; i--) 
        {
            TWeakObjectPtr<AChesses> target = GetChess(i, y);
            if (target != nullptr)
            {
                if (target->GetType() == EChessType::JIANG && target->GetColor() != color)
                {
                    moves.Add(FChessMove2P(Position(x, y), Position(i, y)));
                }
                break;
            }
        }
    }
    else 
    {
        for (int32 i = x + 1; i < 10; i++)
        {
            TWeakObjectPtr<AChesses> target = GetChess(i, y);
            if (target != nullptr)
            {
                if (target->GetType() == EChessType::JIANG && target->GetColor() != color)
                {
                    moves.Add(FChessMove2P(Position(x, y), Position(i, y)));
                }
                break;
            }
        }
    }
}

void UChessBoard2P::GenerateShiMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 士/仕的移动方向：四个斜方向
    int32 directions[4][2] = { {-1, -1}, {-1, 1}, {1, -1}, {1, 1} };

    for (int32 i = 0; i < 4; i++) 
    {
        int32 newX = x + directions[i][0];
        int32 newY = y + directions[i][1];

        // 检查是否在九宫格内
        if (IsInPalace(newX, newY, color)) 
        {
            TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
            if (target == nullptr || target->GetColor() != color) 
            {
                moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
            }
        }
    }
}

void UChessBoard2P::GenerateXiangMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 象/相的移动方向：四个斜方向（走田字）
    int32 directions[4][2] = { {-2, -2}, {-2, 2}, {2, -2}, {2, 2} };

    for (int32 i = 0; i < 4; i++) 
    {
        int32 newX = x + directions[i][0];
        int32 newY = y + directions[i][1];

        if (IsValidPosition(newX, newY))
        {
            // 检查是否过河
            if ((color == EChessColor::BLACK && newX >= 5) || (color == EChessColor::RED && newX <= 4))
            {
                // 检查象眼是否被塞
                int32 eyeX = x + directions[i][0] / 2;
                int32 eyeY = y + directions[i][1] / 2;

                if (GetChess(eyeX, eyeY) == nullptr)
                {
                    TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
                    if (target == nullptr || target->GetColor() != color)
                    {
                        moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
                    }
                }
            }
        }
    }
}

void UChessBoard2P::GenerateMaMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 马/傌的移动方向：八个方向（走日字）
    int32 directions[8][2] = { {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                           {1, -2}, {1, 2}, {2, -1}, {2, 1} };
    // 马腿位置
    int32 horseLegs[8][2] = { {-1, 0}, {-1, 0}, {0, -1}, {0, 1},
                          {0, -1}, {0, 1}, {1, 0}, {1, 0} };

    for (int32 i = 0; i < 8; i++) 
    {
        int32 newX = x + directions[i][0];
        int32 newY = y + directions[i][1];

        if (IsValidPosition(newX, newY)) 
        {
            // 检查马腿是否被绊
            int32 legX = x + horseLegs[i][0];
            int32 legY = y + horseLegs[i][1];

            if (GetChess(legX, legY) == nullptr) 
            {
                TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
                if (target == nullptr || target->GetColor() != color) 
                {
                    moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
                }
            }
        }
    }
}

void UChessBoard2P::GenerateJvMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 车/俥的移动方向：上、下、左、右
    int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

    for (int32 i = 0; i < 4; i++) 
    {
        int32 step = 1;
        while (true)
        {
            int32 newX = x + directions[i][0] * step;
            int32 newY = y + directions[i][1] * step;

            if (!IsValidPosition(newX, newY))
            {
                break;
            }

            TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
            if (target == nullptr)
            {
                moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
            }
            else
            {
                if (target->GetColor() != color)
                {
                    moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
                }
                break;
            }

            step++;
        }
    }
}

void UChessBoard2P::GeneratePaoMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 炮/砲的移动方向：上、下、左、右
    int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

    for (int32 i = 0; i < 4; i++) 
    {
        int32 step = 1;
        bool foundPiece = false;

        while (true)
        {
            int32 newX = x + directions[i][0] * step;
            int32 newY = y + directions[i][1] * step;

            if (!IsValidPosition(newX, newY))
            {
                break;
            }

            TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
            if (!foundPiece)
            {
                if (target == nullptr)
                {
                    moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
                }
                else
                {
                    foundPiece = true;
                }
            }
            else {
                if (target != nullptr)
                {
                    if (target->GetColor() != color)
                    {
                        moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
                    }
                    break;
                }
            }

            step++;
        }
    }
}

void UChessBoard2P::GenerateBingMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 兵/卒的移动方向
    TArray<TPair<int32, int32>> directions;

    if (color == EChessColor::BLACK) 
    {
        directions.Add({ -1, 0 });  // 黑方向下移动
        if (x <= 4)  // 过河后可以左右移动
        {
            directions.Add({ 0, -1 });
            directions.Add({ 0, 1 });
        }
    }
    else 
    {
        directions.Add({ 1, 0 });  // 红方向上移动
        if (x >= 5) // 过河后可以左右移动
        {
            directions.Add({ 0, -1 });
            directions.Add({ 0, 1 });
        }
    }

    for (TPair<int32, int32> dir : directions) 
    {
        int32 newX = x + dir.Key;
        int32 newY = y + dir.Value;

        if (IsValidPosition(newX, newY))
        {
            TWeakObjectPtr<AChesses> target = GetChess(newX, newY);
            if (target == nullptr || target->GetColor() != color)
            {
                moves.Add(FChessMove2P(Position(x, y), Position(newX, newY)));
            }
        }
    }
}

bool UChessBoard2P::IsCheckmate(EChessColor color)
{
    // 简化实现：检查是否无棋可走
    TArray<FChessMove2P> moves = GenerateAllMoves(color);
    return moves.IsEmpty();
}
