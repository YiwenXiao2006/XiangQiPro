// Copyright 2026 Ultimate Player All Rights Reserved.


#include "AI2P.h"
#include "ChessBoard2P.h"
#include "../Chess/Chesses.h"
#include "../Util/TacticsLibrary2P.h"
#include <Kismet/GameplayStatics.h>

UAI2P::UAI2P()
{
}

void UAI2P::BeginDestroy()
{
    // 释放内存
    if (TacticsLibrary.IsValid())
    {
        TacticsLibrary->RemoveFromRoot();
        TacticsLibrary.Reset();
    }
    Super::BeginDestroy();
}

// 初始化战术库
void UAI2P::InitializeTacticsLibrary()
{
    if (TacticsLibrary.IsValid() && board2P.IsValid())
    {
        TacticsLibrary->SetBoard(board2P);
    }
}

// 评估战术价值
int32 UAI2P::EvaluateTactics(const FChessMove2P& move)
{
    if (!TacticsLibrary.IsValid())
    {
        ULogger::LogWarning(TEXT("UAI2P::EvaluateTactics: Tactics library is not available, because TacticsLibrary is nullptr!"));
        return 0;
    }

    FString tacticName;
    int32 tacticScore = 0;

    if (TacticsLibrary->DetectTactics(move, EChessColor::BLACK, tacticName, tacticScore))
    {
        // 记录战术检测结果
        //ULogger::Log(FString::Printf(TEXT("Detected tactic: %s with score: %d"), *tacticName, tacticScore));
        return tacticScore;
    }

    return 0;
}

void UAI2P::SetDepth(int32 Depth)
{
    maxDepth = Depth;
}

void UAI2P::SetBoard(TWeakObjectPtr<ChessBoard2P> newBoard)
{
    board2P = newBoard;
    TacticsLibrary = NewObject<UTacticsLibrary2P>();
    TacticsLibrary->AddToRoot();
    InitializeTacticsLibrary(); // 初始化战术库
}

void UAI2P::StopThinkingImmediately()
{
    timeLimit = 0;
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
    
    // 计算棋子价值和特殊位置价值
    int32 shiCount = 0;  // 士的数量
    int32 xiangCount = 0; // 象的数量
    int32 defensiveValue = 0; // 防守价值

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
            if (chess.IsValid())
            {
                int32 value = chessValues[(int32)chess->GetType()];

                // 特别关注士和象
                if (chess->GetType() == EChessType::SHI && chess->GetColor() == color)
                {
                    shiCount++;
                    value += 50; // 士的额外防守价值
                    value += EvaluateShiPosition(i, j, color);
                }
                else if (chess->GetType() == EChessType::XIANG && chess->GetColor() == color)
                {
                    xiangCount++;
                    value += 40; // 象的额外防守价值
                    value += EvaluateXiangPosition(i, j, color);
                }
                else if ((chess->GetType() == EChessType::JV ||
                    chess->GetType() == EChessType::MA ||
                    chess->GetType() == EChessType::PAO) &&
                    chess->GetColor() == EChessColor::BLACK)
                {
                    // 进攻棋子在防守位置也有价值
                    if (IsInDefensivePosition(i, j, color))
                    {
                        value += 20;
                    }
                }

                if (chess->GetColor() == color)
                {
                    score += value;
                    defensiveValue += CalculateDefensiveContribution(i, j, chess);
                }
                else
                {
                    score -= value;
                }
            }
        }
    }

    // 士和象的协同防守奖励
    score += EvaluateShiXiangCooperation(shiCount, xiangCount, color);

    // 增加防守价值的权重
    score += defensiveValue / 2;

    // 添加局面评估因素
    score += EvaluateMobility(color) - EvaluateMobility(colorOtherSide);
    score += EvaluateThreats(color) - EvaluateThreats(colorOtherSide);
    score += EvaluateKingSafety(color) - EvaluateKingSafety(colorOtherSide);
    score += EvaluateDefensiveStructure(color) - EvaluateDefensiveStructure(colorOtherSide);
    
    // 将帅面对面情况评估（非常重要！）
    if (board2P->AreKingsFacingEachOther())
    {
        // 如果当前轮到AI走棋，且将帅面对面，AI有极大优势
        if (color == EChessColor::BLACK) // 假设AI是黑方
        {
            score += 800; // 巨大优势，因为可以立即吃掉对方将/帅
        }
        else
        {
            score -= 800; // 巨大劣势
        }
    }

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

    if (targetChess.IsValid() && targetChess->GetType() == EChessType::JIANG &&
        targetChess->GetColor() != movingChess->GetColor())
    {
        // 这是将死对方的走法，给予最高奖励
        score += 10000;

        // 额外检查是否是将帅直接面对面的吃法
        if (board2P->AreKingsFacingEachOther())
        {
            score += 5000; // 额外奖励
        }

        return score; // 直接返回，不再评估其他因素
    }

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

    // 2. 特别奖励士和象的防守走法
    if (movingChess.IsValid())
    {
        // 士的走法奖励
        if (movingChess->GetType() == EChessType::SHI && movingChess->GetColor() == EChessColor::BLACK)
        {
            score += EvaluateShiMove(move, movingChess);
        }
        // 象的走法奖励
        else if (movingChess->GetType() == EChessType::XIANG && movingChess->GetColor() == EChessColor::BLACK)
        {
            score += EvaluateXiangMove(move, movingChess);
        }
        // 车、马、炮的进攻走法适当降低优先级（鼓励防守）
        else if ((movingChess->GetType() == EChessType::JV ||
            movingChess->GetType() == EChessType::MA ||
            movingChess->GetType() == EChessType::PAO) &&
            movingChess->GetColor() == EChessColor::BLACK)
        {
            score += EvaluateAttackMove(move, movingChess);
        }
    }

    // 3. 吃子价值（MVV-LVA）
    if (targetChess.IsValid())
    {
        int32 victimValue = chessValues[(int32)targetChess->GetType()];
        int32 attackerValue = movingChess.IsValid() ? chessValues[(int32)movingChess->GetType()] : 0;

        score += victimValue * 10 - attackerValue;
    }

    // 4. 走法安全性：走法后是否处于安全位置
    score += EvaluateMoveSafety(move);

    // 5. 位置价值：根据棋子类型和位置给予奖励
    score += EvaluatePositionValue(move, movingChess);

    // 6. 威胁评估：这个走法是否威胁对方重要棋子
    score += EvaluateThreatAfterMove(move, movingChess);

    // 7. 战术评估：检测经典象棋战术
    score += EvaluateTactics(move);

    return score;
}

// 评估士的走法
int32 UAI2P::EvaluateShiMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> shi)
{
    int32 score = 0;

    // 士在九宫格内移动的奖励
    if (board2P->IsInPalace(move.to.X, move.to.Y, EChessColor::BLACK))
    {
        score += 30;
    }
    else
    {
        score -= 50; // 士离开九宫格严重惩罚
    }

    // 士移动到将/帅附近的奖励
    int32 kingX = 0, kingY = 4; // 黑将位置
    int32 distanceToKing = FMath::Abs(move.to.X - kingX) + FMath::Abs(move.to.Y - kingY);

    if (distanceToKing == 1)
    {
        score += 20; // 紧贴将/帅的防守位置
    }

    // 检查这个走法是否阻挡了对方的攻击
    if (IsBlockingAttack(move, EChessColor::BLACK))
    {
        score += 40; // 阻挡攻击的奖励
    }

    return score;
}

// 评估象的走法
int32 UAI2P::EvaluateXiangMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> xiang)
{
    int32 score = 0;

    // 象在己方半场移动的奖励
    if (move.to.X >= 0 && move.to.X <= 4)
    {
        score += 25;
    }
    else
    {
        score -= 40; // 象离开己方半场惩罚
    }

    // 象移动到中心位置的奖励
    if (move.to.Y >= 3 && move.to.Y <= 5)
    {
        score += 15;
    }

    // 检查象眼是否通畅
    if (IsXiangEyeClear(move.to.X, move.to.Y))
    {
        score += 10;
    }

    // 检查这个走法是否保护了重要位置
    if (IsProtectingKeySquare(move, EChessColor::BLACK))
    {
        score += 30;
    }

    return score;
}

// 评估进攻棋子的走法（适当降低优先级）
int32 UAI2P::EvaluateAttackMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> attacker)
{
    int32 score = 0;

    // 适当降低车马炮的走法优先级，鼓励防守
    score -= 15;

    // 但如果这个走法有重要威胁，还是给予适当奖励
    TWeakObjectPtr<AChesses> target = board2P->GetChess(move.to.X, move.to.Y);
    if (target.IsValid())
    {
        int32 chessValues[8] = { 0, 10000, 200, 200, 400, 900, 450, 100 };
        int32 targetValue = chessValues[(int32)target->GetType()];

        if (targetValue >= 400) // 攻击重要棋子
        {
            score += 20;
        }
    }

    return score;
}

// 评估走法后的位置价值
int32 UAI2P::EvaluatePositionValue(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess)
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

// 评估士的位置价值
int32 UAI2P::EvaluateShiPosition(int32 x, int32 y, EChessColor color)
{
    int32 score = 0;

    // 士应该在九宫格内，靠近将/帅的位置最有价值
    if (!board2P->IsInPalace(x, y, color))
        return -50; // 士离开九宫格惩罚

    // 将/帅的位置（黑将在(0,4)附近，红帅在(9,4)附近）
    int32 kingX = (color == EChessColor::BLACK) ? 0 : 9;
    int32 kingY = 4;

    // 距离将/帅越近，价值越高
    int32 distance = FMath::Abs(x - kingX) + FMath::Abs(y - kingY);
    score += (2 - distance) * 15; // 最大距离为2（士的移动范围）

    // 士在将/帅的斜前方位置最有防守价值
    if ((color == EChessColor::BLACK && ((x == 1 && y == 3) || (x == 1 && y == 5))) ||
        (color == EChessColor::RED && ((x == 8 && y == 3) || (x == 8 && y == 5))))
    {
        score += 20;
    }

    return score;
}

// 评估象的位置价值
int32 UAI2P::EvaluateXiangPosition(int32 x, int32 y, EChessColor color)
{
    int32 score = 0;

    // 象应该在己方半场
    bool inHomeHalf = (color == EChessColor::BLACK) ? (x >= 0 && x <= 4) : (x >= 5 && x <= 9);
    if (!inHomeHalf)
        return -30; // 象离开己方半场惩罚

    // 象在中心位置（保护中路）更有价值
    if (y >= 3 && y <= 5)
    {
        score += 15;
    }

    // 象在边路位置价值较低
    if (y == 1 || y == 7)
    {
        score -= 10;
    }

    // 检查象眼是否通畅
    if (IsXiangEyeClear(x, y))
    {
        score += 10;
    }

    return score;
}

// 检查象眼是否通畅
bool UAI2P::IsXiangEyeClear(int32 x, int32 y)
{
    // 象的四个可能移动方向对应的象眼位置
    TArray<TPair<int32, int32>> directions = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    for (const auto& dir : directions)
    {
        int32 eyeX = x + dir.Key;
        int32 eyeY = y + dir.Value;

        if (board2P->IsValidPosition(eyeX, eyeY))
        {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(eyeX, eyeY);
            if (chess.IsValid())
            {
                return false; // 象眼被塞
            }
        }
    }

    return true;
}

// 评估士和象的协同防守
int32 UAI2P::EvaluateShiXiangCooperation(int32 shiCount, int32 xiangCount, EChessColor color)
{
    int32 score = 0;

    // 完整的士象结构奖励
    if (shiCount == 2 && xiangCount == 2)
    {
        score += 50; // 完整的士象全有，防守稳固
    }
    else if (shiCount >= 1 && xiangCount >= 1)
    {
        score += 20; // 至少有士有象
    }

    // 缺少士或象的惩罚
    if (shiCount == 0) score -= 30;
    if (xiangCount == 0) score -= 25;

    return score;
}

// 评估防守结构
int32 UAI2P::EvaluateDefensiveStructure(EChessColor color)
{
    int32 score = 0;

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

    if (kingX == -1) return 0;

    // 检查将/帅周围的防守棋子
    int32 shiDefenders = 0;
    int32 xiangDefenders = 0;

    // 检查士的防守位置
    TArray<TPair<int32, int32>> shiPositions;
    if (color == EChessColor::BLACK)
    {
        shiPositions = { {0,3}, {0,5}, {1,4}, {2,3}, {2,5} };
    }
    else
    {
        shiPositions = { {7,3}, {7,5}, {8,4}, {9,3}, {9,5} };
    }

    for (const auto& pos : shiPositions)
    {
        TWeakObjectPtr<AChesses> chess = board2P->GetChess(pos.Key, pos.Value);
        if (chess.IsValid() && chess->GetType() == EChessType::SHI && chess->GetColor() == color)
        {
            shiDefenders++;
        }
    }

    // 检查象的防守位置
    TArray<TPair<int32, int32>> xiangPositions;
    if (color == EChessColor::BLACK)
    {
        xiangPositions = { {0,2}, {0,6}, {2,0}, {2,4}, {2,8}, {4,2}, {4,6} };
    }
    else
    {
        xiangPositions = { {5,2}, {5,6}, {7,0}, {7,4}, {7,8}, {9,2}, {9,6} };
    }

    for (const auto& pos : xiangPositions)
    {
        TWeakObjectPtr<AChesses> chess = board2P->GetChess(pos.Key, pos.Value);
        if (chess.IsValid() && chess->GetType() == EChessType::XIANG && chess->GetColor() == color)
        {
            xiangDefenders++;

            // 检查象眼是否通畅
            int32 eyeX = (pos.Key + kingX) / 2;
            int32 eyeY = (pos.Value + kingY) / 2;
            TWeakObjectPtr<AChesses> eyeChess = board2P->GetChess(eyeX, eyeY);
            if (!eyeChess.IsValid())
            {
                score += 5; // 象眼通畅奖励
            }
        }
    }

    score += shiDefenders * 10;
    score += xiangDefenders * 8;

    return score;
}

// 评估走法后对对方的威胁
int32 UAI2P::EvaluateThreatAfterMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> movingChess)
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
}// 检查走法是否阻挡了对方的攻击
bool UAI2P::IsBlockingAttack(const FChessMove2P& move, EChessColor color)
{
    if (!board2P.IsValid()) return false;

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查是否减少了对方的攻击路线
    EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

    // 找到己方将/帅的位置
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

    if (kingX == -1)
    {
        board2P->UndoTestMove(move, capturedChess);
        return false;
    }

    // 检查对方是否能攻击将/帅
    bool canAttackKingAfterMove = false;
    TArray<FChessMove2P> opponentMoves = board2P->GenerateAllMoves(opponentColor);
    for (const FChessMove2P& oppMove : opponentMoves)
    {
        if (oppMove.to.X == kingX && oppMove.to.Y == kingY)
        {
            canAttackKingAfterMove = true;
            break;
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, capturedChess);

    // 检查原始局面对方是否能攻击将/帅
    bool canAttackKingBeforeMove = false;
    TArray<FChessMove2P> originalOpponentMoves = board2P->GenerateAllMoves(opponentColor);
    for (const FChessMove2P& oppMove : originalOpponentMoves)
    {
        if (oppMove.to.X == kingX && oppMove.to.Y == kingY)
        {
            canAttackKingBeforeMove = true;
            break;
        }
    }

    // 如果走法前能被攻击，走法后不能被攻击，说明这个走法阻挡了攻击
    return canAttackKingBeforeMove && !canAttackKingAfterMove;
}

// 检查走法是否保护了关键位置
bool UAI2P::IsProtectingKeySquare(const FChessMove2P& move, EChessColor color)
{
    if (!board2P.IsValid()) return false;

    // 关键位置：将/帅周围的九宫格位置
    TArray<TPair<int32, int32>> keySquares;
    if (color == EChessColor::BLACK)
    {
        keySquares = {
            {0,3}, {0,4}, {0,5},
            {1,3}, {1,4}, {1,5},
            {2,3}, {2,4}, {2,5}
        };
    }
    else
    {
        keySquares = {
            {7,3}, {7,4}, {7,5},
            {8,3}, {8,4}, {8,5},
            {9,3}, {9,4}, {9,5}
        };
    }

    // 模拟走法
    TWeakObjectPtr<AChesses> capturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 检查移动后的棋子是否能保护关键位置
    TWeakObjectPtr<AChesses> movingChess = board2P->GetChess(move.to.X, move.to.Y);
    if (!movingChess.IsValid())
    {
        board2P->UndoTestMove(move, capturedChess);
        return false;
    }

    bool isProtecting = false;

    for (const auto& square : keySquares)
    {
        // 检查这个棋子是否能攻击/保护关键位置
        if (CanAttackPosition(move.to.X, move.to.Y, square.Key, square.Value, color))
        {
            // 检查这个关键位置是否被对方威胁
            EChessColor opponentColor = (color == EChessColor::BLACK) ? EChessColor::RED : EChessColor::BLACK;

            // 检查原始局面这个关键位置是否被对方攻击
            board2P->UndoTestMove(move, capturedChess);
            bool wasThreatened = false;
            TArray<FChessMove2P> originalOpponentMoves = board2P->GenerateAllMoves(opponentColor);
            for (const FChessMove2P& oppMove : originalOpponentMoves)
            {
                if (oppMove.to.X == square.Key && oppMove.to.Y == square.Value)
                {
                    wasThreatened = true;
                    break;
                }
            }

            // 重新应用走法
            board2P->MakeTestMove(move);

            // 检查走法后这个关键位置是否还被对方攻击
            bool isThreatened = false;
            TArray<FChessMove2P> opponentMoves = board2P->GenerateAllMoves(opponentColor);
            for (const FChessMove2P& oppMove : opponentMoves)
            {
                if (oppMove.to.X == square.Key && oppMove.to.Y == square.Value)
                {
                    isThreatened = true;
                    break;
                }
            }

            // 如果走法前被威胁，走法后不再被威胁，说明这个走法保护了关键位置
            if (wasThreatened && !isThreatened)
            {
                isProtecting = true;
                break;
            }
        }
    }

    board2P->UndoTestMove(move, capturedChess);

    return isProtecting;
}

// 检查棋子是否能攻击指定位置
bool UAI2P::CanAttackPosition(int32 fromX, int32 fromY, int32 toX, int32 toY, EChessColor attackerColor)
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
bool UAI2P::IsMoveProtectingKing(const FChessMove2P& move, EChessColor color)
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

// 检查棋子是否在防守位置
bool UAI2P::IsInDefensivePosition(int32 x, int32 y, EChessColor color)
{
    // 防守位置：己方半场，特别是靠近将/帅的位置
    if (color == EChessColor::BLACK)
    {
        return x <= 4; // 黑方半场
    }
    else
    {
        return x >= 5; // 红方半场
    }
}

// 计算棋子的防守贡献
int32 UAI2P::CalculateDefensiveContribution(int32 x, int32 y, TWeakObjectPtr<AChesses> chess)
{
    if (!chess.IsValid()) return 0;

    int32 contribution = 0;

    // 士和象的防守贡献最大
    if (chess->GetType() == EChessType::SHI)
    {
        contribution = 60;
    }
    else if (chess->GetType() == EChessType::XIANG)
    {
        contribution = 50;
    }
    else if (chess->GetType() == EChessType::JV)
    {
        contribution = 30; // 车在防守中也有重要作用
    }
    else if (chess->GetType() == EChessType::MA || chess->GetType() == EChessType::PAO)
    {
        contribution = 20;
    }

    // 根据位置调整贡献值
    if (IsInDefensivePosition(x, y, chess->GetColor()))
    {
        contribution += 10;
    }

    return contribution;
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

        // 特别优先考虑士和象的走法
        TWeakObjectPtr<AChesses> chessA = board2P->GetChess(moveA.from.X, moveA.from.Y);
        TWeakObjectPtr<AChesses> chessB = board2P->GetChess(moveB.from.X, moveB.from.Y);

        if (chessA.IsValid() && chessA->GetColor() == EChessColor::BLACK)
        {
            if (chessA->GetType() == EChessType::SHI || chessA->GetType() == EChessType::XIANG)
            {
                scoreA += 50; // 士和象的走法额外加分
            }
        }

        if (chessB.IsValid() && chessB->GetColor() == EChessColor::BLACK)
        {
            if (chessB->GetType() == EChessType::SHI || chessB->GetType() == EChessType::XIANG)
            {
                scoreB += 50;
            }
        }

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

    timeLimit = (maxDepth + 2) * 1500;
    Timer.Start();

    // 首先检查当前是否被将军
    bool inCheck = board2P->IsKingInCheck(EChessColor::BLACK);

    TArray<FChessMove2P> moves = board2P->GenerateAllMoves(EChessColor::BLACK);
    TWeakObjectPtr<AChesses> movedChess;

    // 如果被将军，使用专门的保将搜索
    if (inCheck)
    {
        ULogger::Log(TEXT("UAI2P::GetBestMove: AI is in check! Prioritizing king safety moves."));
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
        ULogger::LogWarning(TEXT("UAI2P::GetBestMove: No safe moves found! AI may move into check."));
        safeMoves = moves; // 使用所有走法，包括不安全的
    }
    else
    {
        moves = safeMoves; // 使用安全走法
        ULogger::Log(FString::Printf(TEXT("UAI2P::GetBestMove: Filtered to %d safe moves"), safeMoves.Num()));
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
                ULogger::Log(FString::Printf(TEXT("UAI2P::GetBestMove: Depth %d: Found safe move with value %d"), depth, currentBestValue));
            }
            else
            {
                ULogger::LogWarning(FString::Printf(TEXT("UAI2P::GetBestMove: Depth %d: Best move causes check! Value: %d"), depth, currentBestValue));
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

        // 能直接吃掉红方的帅
        if (capturedChess.IsValid() && capturedChess->GetColor() == EChessColor::RED && capturedChess->GetType() == EChessType::JIANG)
        {
            bestMove = move;
            return board2P->GetChess(move.from.X, move.from.Y); // 不需要搜索了,可以直接胜利
        }

        board2P->MakeTestMove(move);

        bool stillInCheck = board2P->IsKingInCheck(EChessColor::BLACK);

        board2P->UndoTestMove(move, capturedChess);

        // 破除将军状态
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

    ULogger::Log(FString::Printf(TEXT("UAI2P::GetBestMoveWhenInCheck: Selected escape move with value %d"), bestEscapeValue));

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
