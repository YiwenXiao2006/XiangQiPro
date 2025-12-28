// Copyright 2026 Ultimate Player All Rights Reserved.

#include "TacticsLibrary2P.h"
#include "../Util/ChessInfo.h"
#include "../GameObject/ChessBoard2P.h"
#include "../Chess/Chesses.h"

UTacticsLibrary2P::UTacticsLibrary2P()
{
}

void UTacticsLibrary2P::SetBoard(TWeakObjectPtr<UChessBoard2P> newBoard)
{
    board2P = newBoard;
}

bool UTacticsLibrary2P::DetectTactics(const FChessMove2P& move, EChessColor color, FString& tacticName, int32& tacticScore)
{
    if (!board2P.IsValid()) return false;

    tacticScore = 0;
    int32 maxScore = 0;
    FString bestTactic = "";

    // 检测各种战术
    int32 score;

    if (DetectFork(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "Fork";
    }

    if (DetectPin(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "Pin";
    }

    if (DiscoveredAttack(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "DiscoveredAttack";
    }

    if (Skewer(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "Skewer";
    }

    if (DoubleCheck(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "DoubleCheck";
    }

    if (Sacrifice(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "Sacrifice";
    }

    if (DefensiveTactic(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "DefensiveTactic";
    }

    if (PositionalTactic(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "PositionalTactic";
    }

    // 将帅面对面战术检测
    if (DetectKingFaceOff(move, color, score) && score > maxScore)
    {
        maxScore = score;
        bestTactic = "KingFaceOff";
    }

    if (maxScore > 0)
    {
        tacticName = bestTactic;
        tacticScore = maxScore;
        return true;
    }

    return false;
}

bool UTacticsLibrary2P::DetectKingFaceOff(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);
    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);

    if (!movingChess.IsValid()) return false;

    // 检查是否是将帅直接攻击
    if (movingChess->GetType() == EChessType::JIANG &&
        targetChess.IsValid() && targetChess->GetType() == EChessType::JIANG &&
        targetChess->GetColor() != movingChess->GetColor())
    {
        // 检查是否是将帅面对面且中间无阻挡
        if (move.from.Y == move.to.Y) // 同一列
        {
            int32 minX = FMath::Min(move.from.X, move.to.X);
            int32 maxX = FMath::Max(move.from.X, move.to.X);
            bool hasPieceBetween = false;

            for (int32 x = minX + 1; x < maxX; x++)
            {
                TWeakObjectPtr<AChesses> chess = board2P->GetChess(x, move.from.Y);
                if (chess.IsValid() && chess->GetType() != EChessType::EMPTY)
                {
                    hasPieceBetween = true;
                    break;
                }
            }

            if (!hasPieceBetween)
            {
                // 将帅面对面攻击，这是绝杀战术
                score = 200; // 给予高分奖励
                return true;
            }
        }
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::DetectFork(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查移动后的棋子是否能同时攻击多个重要棋子
    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    TArray<TPair<EChessType, Position>> threatenedPieces;
    int32 totalThreatValue = 0;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> target = board2P->GetChess(i, j);
            if (target.IsValid() && target->GetColor() == opponentColor && IsPieceValuable(target->GetType()))
            {
                if (CanPieceCapture(move.to.X, move.to.Y, i, j, color))
                {
                    threatenedPieces.Add(TPair<EChessType, Position>(target->GetType(), Position(i, j)));
                    totalThreatValue += GetPieceValue(target->GetType());
                }
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    // 如果同时威胁多个棋子（至少2个），且总价值较高
    if (threatenedPieces.Num() >= 2)
    {
        // 计算捉双分数：基于威胁的棋子价值和数量
        score = totalThreatValue / 10 + threatenedPieces.Num() * 20;

        // 特别奖励捉车、马、炮等重要棋子
        for (const auto& piece : threatenedPieces)
        {
            if (piece.Key == EChessType::JV || piece.Key == EChessType::MA || piece.Key == EChessType::PAO)
            {
                score += 30;
            }
        }

        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::DetectPin(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 检查这个走法是否创造了牵制
    // 牵制：移动棋子后，让对方的某个棋子无法移动（因为移动会暴露更重要的棋子）

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 查找对方的棋子，检查是否被牵制
    int32 pinScore = 0;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> opponentPiece = board2P->GetChess(i, j);
            if (!opponentPiece.IsValid() || opponentPiece->GetColor() != opponentColor)
                continue;

            // 检查这个棋子后面是否有更重要的棋子
            // 这里简化实现：检查直线方向上的棋子关系
            TArray<FChessMove2P> opponentMoves = board2P->GenerateMovesForChess(i, j, opponentPiece);

            // 如果这个棋子的移动受到限制（比如车被炮牵制）
            bool isPinned = false;
            EChessType valuablePieceType = EChessType::EMPTY;

            // 检查直线方向
            int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
            for (int32 d = 0; d < 4; d++)
            {
                int32 step = 1;
                while (true)
                {
                    int32 checkX = i + directions[d][0] * step;
                    int32 checkY = j + directions[d][1] * step;

                    if (!board2P->IsValidPosition(checkX, checkY))
                        break;

                    TWeakObjectPtr<AChesses> behindPiece = board2P->GetChess(checkX, checkY);
                    if (behindPiece.IsValid())
                    {
                        if (behindPiece->GetColor() == opponentColor &&
                            GetPieceValue(behindPiece->GetType()) > GetPieceValue(opponentPiece->GetType()))
                        {
                            // 发现牵制！
                            isPinned = true;
                            valuablePieceType = behindPiece->GetType();
                            break;
                        }
                        break; // 有棋子阻挡，停止检查
                    }
                    step++;
                }
                if (isPinned) break;
            }

            if (isPinned)
            {
                pinScore += GetPieceValue(valuablePieceType) / 20 + 25;
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    if (pinScore > 0)
    {
        score = pinScore;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::DiscoveredAttack(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 闪击：移动一个棋子后，露出后面的棋子进行攻击

    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);
    if (!movingChess.IsValid()) return false;

    // 模拟走法前，检查移动的棋子后面是否有己方棋子
    bool hasAttackBehind = false;
    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 检查直线方向
    int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
    for (int32 d = 0; d < 4; d++)
    {
        int32 step = 1;
        while (true)
        {
            int32 checkX = move.from.X + directions[d][0] * step;
            int32 checkY = move.from.Y + directions[d][1] * step;

            if (!board2P->IsValidPosition(checkX, checkY))
                break;

            TWeakObjectPtr<AChesses> behindPiece = board2P->GetChess(checkX, checkY);
            if (behindPiece.IsValid())
            {
                if (behindPiece->GetColor() == color &&
                    (behindPiece->GetType() == EChessType::JV || behindPiece->GetType() == EChessType::PAO))
                {
                    // 后面有车或炮，可能形成闪击
                    hasAttackBehind = true;
                }
                break;
            }
            step++;
        }
        if (hasAttackBehind) break;
    }

    if (!hasAttackBehind)
    {
        score = 0;
        return false;
    }

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查移动后是否露出了有效的攻击
    int32 attackScore = 0;

    // 重新检查刚才的方向，现在应该可以攻击了
    for (int32 d = 0; d < 4; d++)
    {
        int32 step = 1;
        while (true)
        {
            int32 checkX = move.from.X + directions[d][0] * step;
            int32 checkY = move.from.Y + directions[d][1] * step;

            if (!board2P->IsValidPosition(checkX, checkY))
                break;

            TWeakObjectPtr<AChesses> behindPiece = board2P->GetChess(checkX, checkY);
            if (behindPiece.IsValid())
            {
                if (behindPiece->GetColor() == color)
                {
                    // 检查这个棋子是否能攻击对方重要目标
                    for (int32 i = 0; i < 10; i++)
                    {
                        for (int32 j = 0; j < 9; j++)
                        {
                            TWeakObjectPtr<AChesses> target = board2P->GetChess(i, j);
                            if (target.IsValid() && target->GetColor() == opponentColor &&
                                IsPieceValuable(target->GetType()))
                            {
                                if (CanPieceCapture(checkX, checkY, i, j, color))
                                {
                                    attackScore += GetPieceValue(target->GetType()) / 15 + 20;
                                }
                            }
                        }
                    }
                }
                break;
            }
            step++;
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    if (attackScore > 0)
    {
        score = attackScore;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::Skewer(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 串打：攻击一个棋子，迫使它移动后，攻击它后面的更重要棋子
    // 类似于牵制，但是主动性的

    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);
    if (!targetChess.IsValid() || !IsPieceValuable(targetChess->GetType()))
    {
        score = 0;
        return false;
    }

    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 检查目标棋子后面是否有更重要的棋子
    bool hasValuableBehind = false;
    EChessType valuableType = EChessType::EMPTY;

    // 检查直线方向
    int32 directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
    for (int32 d = 0; d < 4; d++)
    {
        int32 step = 1;
        while (true)
        {
            int32 checkX = move.to.X + directions[d][0] * step;
            int32 checkY = move.to.Y + directions[d][1] * step;

            if (!board2P->IsValidPosition(checkX, checkY))
                break;

            TWeakObjectPtr<AChesses> behindPiece = board2P->GetChess(checkX, checkY);
            if (behindPiece.IsValid())
            {
                if (behindPiece->GetColor() == opponentColor &&
                    GetPieceValue(behindPiece->GetType()) > GetPieceValue(targetChess->GetType()))
                {
                    hasValuableBehind = true;
                    valuableType = behindPiece->GetType();
                    break;
                }
                break; // 有棋子阻挡，停止检查
            }
            step++;
        }
        if (hasValuableBehind) break;
    }

    if (hasValuableBehind)
    {
        // 串打战术分数基于后面棋子的价值
        score = GetPieceValue(valuableType) / 10 + 25;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::DoubleCheck(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 双将：移动后形成两个棋子同时将军

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 找到对方将/帅的位置
    int32 kingX = -1, kingY = -1;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG && chess->GetColor() == opponentColor)
            {
                kingX = i;
                kingY = j;
                break;
            }
        }
        if (kingX != -1) break;
    }

    if (kingX == -1)
    {
        board2P->UndoTestMove(move, capturedChess);
        score = 0;
        return false;
    }

    // 计算有多少个棋子可以攻击将/帅
    int32 checkCount = 0;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> attacker = board2P->GetChess(i, j);
            if (attacker.IsValid() && attacker->GetColor() == color)
            {
                if (CanPieceCapture(i, j, kingX, kingY, color))
                {
                    checkCount++;
                }
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    if (checkCount >= 2)
    {
        // 双将是非常有力的战术
        score = 80 + (checkCount - 2) * 20; // 双将80分，每多一个将再加20分
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::Sacrifice(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 弃子战术：牺牲一个棋子以获得局面优势

    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);
    TWeakObjectPtr<AChesses> targetChess = board2P->GetChess(move.to.X, move.to.Y);

    if (!movingChess.IsValid()) return false;

    // 如果走法是吃子，但不是弃子
    if (targetChess.IsValid())
    {
        score = 0;
        return false;
    }

    int32 movingPieceValue = GetPieceValue(movingChess->GetType());

    // 只有相对有价值的棋子牺牲才算是战术弃子
    if (movingPieceValue < 200) // 兵、士、象的牺牲不算重要弃子
    {
        score = 0;
        return false;
    }

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 评估弃子后的局面优势
    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    int32 positionalAdvantage = 0;

    // 1. 检查是否获得强大的攻击机会
    bool hasStrongAttack = false;

    // 检查是否能攻击对方将/帅
    int32 kingX = -1, kingY = -1;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG && chess->GetColor() == opponentColor)
            {
                kingX = i;
                kingY = j;
                break;
            }
        }
        if (kingX != -1) break;
    }

    if (kingX != -1)
    {
        // 检查攻击将/帅的强度
        TArray<FChessMove2P> threats = GetThreatsAfterMove(move, color);
        for (const FChessMove2P& threat : threats)
        {
            if (threat.to.X == kingX && threat.to.Y == kingY)
            {
                hasStrongAttack = true;
                positionalAdvantage += 50;
                break;
            }
        }
    }

    // 2. 检查是否获得局面控制
    int32 controlBonus = 0;

    // 检查重要位置的控制
    TArray<Position> keyPositions;
    if (color == EChessColor::BLACK)
    {
        // 黑方关心的关键位置：中心、对方半场等
        keyPositions.Add(Position(4, 4)); // 中心
        keyPositions.Add(Position(5, 4)); // 河界
        keyPositions.Add(Position(7, 4)); // 对方九宫附近
    }
    else
    {
        keyPositions.Add(Position(5, 4));
        keyPositions.Add(Position(4, 4));
        keyPositions.Add(Position(2, 4));
    }

    for (const Position& pos : keyPositions)
    {
        if (IsPositionAttacked(pos.X, pos.Y, color))
        {
            controlBonus += 10;
        }
    }

    positionalAdvantage += controlBonus;

    // 3. 计算弃子补偿
    int32 sacrificeScore = 0;
    if (positionalAdvantage > movingPieceValue / 2) // 局面优势至少达到牺牲棋子价值的一半
    {
        sacrificeScore = positionalAdvantage - movingPieceValue / 3;
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    if (sacrificeScore > 0)
    {
        score = sacrificeScore;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::DefensiveTactic(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 防守战术：有效的防守走法

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 检查走法后是否解除了威胁
    bool removesThreat = false;
    int32 defensiveValue = 0;

    // 1. 检查是否解除将军
    bool wasInCheck = board2P->IsKingInCheck(color);
    bool nowInCheck = board2P->IsKingInCheck(color);

    if (wasInCheck && !nowInCheck)
    {
        removesThreat = true;
        defensiveValue += 60; // 解除将军的高分
    }

    // 2. 检查是否保护了重要棋子
    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.to.X, move.to.Y);
    if (movingChess.IsValid())
    {
        // 检查移动后的棋子是否能保护被威胁的己方棋子
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> defendedPiece = board2P->GetChess(i, j);
                if (defendedPiece.IsValid() && defendedPiece->GetColor() == color && IsPieceValuable(defendedPiece->GetType()))
                {
                    // 检查这个棋子是否被对方威胁
                    if (IsPositionAttacked(i, j, opponentColor))
                    {
                        // 检查移动后的棋子是否能保护这个棋子
                        if (CanPieceCapture(move.to.X, move.to.Y, i, j, color) ||
                            (move.to.X == i && move.to.Y == j)) // 直接走到被威胁位置
                        {
                            defensiveValue += GetPieceValue(defendedPiece->GetType()) / 20 + 15;
                            removesThreat = true;
                        }
                    }
                }
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    if (removesThreat)
    {
        score = defensiveValue;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::PositionalTactic(const FChessMove2P& move, EChessColor color, int32& score)
{
    if (!board2P.IsValid()) return false;

    // 局面性战术：改善棋子位置、控制关键点等

    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.from.X, move.from.Y);
    if (!movingChess.IsValid()) return false;

    int32 positionalScore = 0;
    EChessType pieceType = movingChess->GetType();

    // 根据棋子类型评估位置改善
    switch (pieceType)
    {
    case EChessType::MA:
        // 马跳到中心或对方半场有奖励
        if (move.to.Y >= 3 && move.to.Y <= 5) // 中心区域
        {
            positionalScore += 15;
        }
        if ((color == EChessColor::BLACK && move.to.X >= 5) ||
            (color == EChessColor::RED && move.to.X <= 4)) // 对方半场
        {
            positionalScore += 20;
        }
        break;

    case EChessType::JV:
        // 车控制开放线、次底线等
        if (move.to.X == 1 || move.to.X == 8) // 次底线
        {
            positionalScore += 25;
        }
        // 车在对方将门有奖励
        if ((color == EChessColor::BLACK && move.to.X == 8 && (move.to.Y == 3 || move.to.Y == 5)) ||
            (color == EChessColor::RED && move.to.X == 1 && (move.to.Y == 3 || move.to.Y == 5)))
        {
            positionalScore += 30;
        }
        break;

    case EChessType::PAO:
        // 炮在河界、对方兵林有奖励
        if (move.to.X == 4 || move.to.X == 5) // 河界
        {
            positionalScore += 15;
        }
        if ((color == EChessColor::BLACK && move.to.X == 7) ||
            (color == EChessColor::RED && move.to.X == 2)) // 兵林
        {
            positionalScore += 20;
        }
        break;

    case EChessType::BING:
        // 兵过河、占据中心有奖励
        if ((color == EChessColor::BLACK && move.to.X <= 4) ||
            (color == EChessColor::RED && move.to.X >= 5)) // 过河
        {
            positionalScore += 20;
        }
        if (move.to.Y >= 3 && move.to.Y <= 5) // 中心
        {
            positionalScore += 10;
        }
        break;

    default:
        break;
    }

    // 控制关键点的奖励
    TArray<Position> keyPoints;
    if (color == EChessColor::BLACK)
    {
        keyPoints.Add(Position(4, 4)); // 中心
        keyPoints.Add(Position(5, 4)); // 河界
        keyPoints.Add(Position(7, 4)); // 对方将门附近
        keyPoints.Add(Position(8, 3)); // 对方九宫要点
        keyPoints.Add(Position(8, 5));
    }
    else
    {
        keyPoints.Add(Position(5, 4));
        keyPoints.Add(Position(4, 4));
        keyPoints.Add(Position(2, 4));
        keyPoints.Add(Position(1, 3));
        keyPoints.Add(Position(1, 5));
    }

    for (const Position& point : keyPoints)
    {
        if (move.to.X == point.X && move.to.Y == point.Y)
        {
            positionalScore += 25;
            break;
        }
    }

    // 改善棋子协调性的奖励
    // 检查移动后是否与其他棋子形成配合
    int32 coordinationBonus = 0;

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查与其他棋子的配合
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> ally = board2P->GetChess(i, j);
            if (ally.IsValid() && ally->GetColor() == color && ally != movingChess)
            {
                // 检查是否能形成配合攻击
                if ((pieceType == EChessType::JV && ally->GetType() == EChessType::PAO) ||
                    (pieceType == EChessType::PAO && ally->GetType() == EChessType::JV))
                {
                    // 车炮配合
                    coordinationBonus += 15;
                }
                else if ((pieceType == EChessType::MA && ally->GetType() == EChessType::JV) ||
                    (pieceType == EChessType::JV && ally->GetType() == EChessType::MA))
                {
                    // 马车配合
                    coordinationBonus += 12;
                }
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    positionalScore += coordinationBonus;

    if (positionalScore > 0)
    {
        score = positionalScore;
        return true;
    }

    score = 0;
    return false;
}

bool UTacticsLibrary2P::IsPieceValuable(EChessType pieceType)
{
    // 判断棋子是否有重要价值（兵、士、象以外的棋子）
    return pieceType == EChessType::JV || pieceType == EChessType::MA ||
        pieceType == EChessType::PAO || pieceType == EChessType::JIANG;
}

int32 UTacticsLibrary2P::GetPieceValue(EChessType pieceType)
{
    if (pieceType >= EChessType::EMPTY && pieceType <= EChessType::BING)
    {
        return chessValues[(int32)pieceType];
    }
    return 0;
}

bool UTacticsLibrary2P::IsPositionAttacked(int32 x, int32 y, EChessColor attackerColor)
{
    if (!board2P.IsValid()) return false;

    // 检查该位置是否被对方攻击
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> attacker = board2P->GetChess(i, j);
            if (attacker.IsValid() && attacker->GetColor() == attackerColor)
            {
                if (CanPieceCapture(i, j, x, y, attackerColor))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool UTacticsLibrary2P::CanPieceCapture(int32 fromX, int32 fromY, int32 targetX, int32 targetY, EChessColor attackerColor)
{
    if (!board2P.IsValid()) return false;

    TWeakObjectPtr<AChesses> attacker = board2P->GetChess(fromX, fromY);
    if (!attacker.IsValid() || attacker->GetColor() != attackerColor)
        return false;

    // 使用棋盘类的攻击检测功能
    return board2P->CanAttackPosition(fromX, fromY, targetX, targetY, attackerColor);
}

TArray<FChessMove2P> UTacticsLibrary2P::GetThreatsAfterMove(const FChessMove2P& move, EChessColor color)
{
    TArray<FChessMove2P> threats;

    if (!board2P.IsValid()) return threats;

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 生成所有可能的攻击走法
    threats = board2P->GenerateAllMoves(color);

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    return threats;
}