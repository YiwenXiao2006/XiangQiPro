// Copyright 2026 Ultimate Player All Rights Reserved.

#include "AI2P.h"
#include "ChessMLModule.h"
#include "XIANGQIPRO/GameObject/ChessBoard2P.h"
#include "XIANGQIPRO/Chess/Chesses.h"
#include <Kismet/GameplayStatics.h>

UAI2P::UAI2P()
{
}

void UAI2P::SetBoard(TWeakObjectPtr<UChessBoard2P> AIMove2P)
{
    LocalAllChess = AIMove2P->AllChess;
}

bool UAI2P::IsMoveSuicidal(const FChessMove2P& Move, EChessColor AiColor)
{
    TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
    if (!MovedChess.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟走法
    TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
    MakeTestMove(Move, Moved);

    bool bIsSuicidal = false;

    // 检查移动后的棋子是否会被对方低价值棋子吃掉
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& OppMove : OpponentMoves) {
        if (OppMove.to.X == Move.to.X && OppMove.to.Y == Move.to.Y) {
            TWeakObjectPtr<AChesses> Attacker = GetChess(OppMove.from.X, OppMove.from.Y);
            if (Attacker.IsValid()) {
                int32 AttackerValue = GetPieceBaseValue(Attacker->GetType(), GamePhase, OpponentColor);
                int32 MovedValue = GetPieceBaseValue(MovedChess->GetType(), GamePhase, AiColor);

                // 如果被低价值棋子吃掉，且没有补偿，则是送子
                if (AttackerValue < MovedValue && !IsPieceRooted(Move.to.X, Move.to.Y, AiColor)) {
                    bIsSuicidal = true;
                    break;
                }
            }
        }
    }

    UndoTestMove(Move, Moved, Captured);
    return bIsSuicidal;
}

// 过滤车/炮的无效移动（自将、攻击有根且无收益、超出棋盘等）
TArray<FChessMove2P> UAI2P::FilterInvalidMoves(const TArray<FChessMove2P>& RawMoves, EChessColor AiColor, EChessType PieceType)
{
    TArray<FChessMove2P> ValidMoves;
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    for (const FChessMove2P& Move : RawMoves)
    {
        // 1. 过滤超出棋盘的移动
        if (Move.to.X < 0 || Move.to.X >= 10 || Move.to.Y < 0 || Move.to.Y >= 9)
            continue;

        // 2. 过滤移动后自将的情况（核心无效移动）
        if (IsInCheckAfterMove(Move, AiColor))
            continue;

        // 3. 过滤攻击「有根且低价值」棋子的移动（车/炮专属）
        TWeakObjectPtr<AChesses> TargetChess = GetChess(Move.to.X, Move.to.Y);
        if (TargetChess.IsValid())
        {
            // 目标是对方棋子：判断是否有根，且价值低于己方车/炮 → 无效移动
            if (TargetChess->GetColor() == OpponentColor)
            {
                bool bTargetRooted = IsPieceRooted(Move.to.X, Move.to.Y, OpponentColor);
                int32 TargetValue = GetPieceBaseValue(TargetChess->GetType(), GamePhase, AiColor);
                int32 SelfValue = GetPieceBaseValue(PieceType, GamePhase, AiColor);

                // 有根且目标价值 < 自身价值 → 攻击无收益，过滤
                if (bTargetRooted && TargetValue < SelfValue)
                    continue;
            }
            // 目标是己方棋子 → 无效（象棋不能吃己方）
            else
            {
                continue;
            }
        }

        // 4. 炮的特殊过滤：空移动（无炮架）且无战术价值的情况
        if (PieceType == EChessType::PAO && !TargetChess.IsValid())
        {
            // 更智能的炮移动评估
            bool bIsMeaningfulMove = false;

            // 条件1：移动到战略位置（中线、对方半场）
            if (Move.to.Y == 4) bIsMeaningfulMove = true; // 中线
            if ((AiColor == EChessColor::REDCHESS && Move.to.X <= 4) ||
                (AiColor == EChessColor::BLACKCHESS && Move.to.X >= 5)) {
                bIsMeaningfulMove = true; // 对方半场
            }

            // 条件2：移动到能形成威胁的位置
            TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
            if (MovedChess.IsValid()) {
                // 模拟移动后检查是否能威胁对方棋子
                TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
                TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
                MakeTestMove(Move, Moved);

                TArray<FChessMove2P> ThreatMoves = GenerateMovesForChess(Move.to.X, Move.to.Y, MovedChess);
                for (const FChessMove2P& ThreatMove : ThreatMoves) {
                    TWeakObjectPtr<AChesses> ThreatTarget = GetChess(ThreatMove.to.X, ThreatMove.to.Y);
                    if (ThreatTarget.IsValid() && ThreatTarget->GetColor() == OpponentColor) {
                        bIsMeaningfulMove = true;
                        break;
                    }
                }

                UndoTestMove(Move, Moved, Captured);
            }

            // 条件3：移动到能配合其他棋子的位置
            if (!bIsMeaningfulMove) {
                // 检查是否能与车、马等形成配合
                int32 SynergyScore = EvaluateAttackSynergy(Move, AiColor);
                if (SynergyScore > 200) {
                    bIsMeaningfulMove = true;
                }
            }

            if (!bIsMeaningfulMove) {
                continue; // 过滤无意义的炮移动
            }
        }

        // 所有校验通过，保留有效移动
        ValidMoves.Add(Move);
    }

    return ValidMoves;
}

// ===================== 经典战术识别与评估 =====================
// 评估当前局面最优战术（按游戏阶段优先级选择）
FTacticEvalResult UAI2P::EvaluateBestTactic(EChessColor AiColor)
{
    FTacticEvalResult BestTactic;

    // 按游戏阶段优先级评估战术
    TArray<FTacticEvalResult> TacticResults;
    if (GamePhase == EChessGamePhase::Opening)
    {
        // 开局：优先中路突破
        TacticResults.Add(RecognizeZhongLuTuPo(AiColor));
        TacticResults.Add(RecognizeChenDiPao(AiColor));
    }
    else if (GamePhase == EChessGamePhase::Midgame)
    {
        // 中局：优先卧槽马、沉底炮
        TacticResults.Add(RecognizeWoCaoMa(AiColor));
        TacticResults.Add(RecognizeChenDiPao(AiColor));
        TacticResults.Add(RecognizeZhongLuTuPo(AiColor));
    }
    else
    {
        // 残局：优先双车错、兵线推进
        TacticResults.Add(RecognizeShuangCheCuo(AiColor));
        TacticResults.Add(RecognizeBingXianTuiJin(AiColor));
        TacticResults.Add(RecognizeWoCaoMa(AiColor));
    }

    // 选择可行性最高的战术
    for (const FTacticEvalResult& Result : TacticResults)
    {
        if (Result.FeasibilityScore > BestTactic.FeasibilityScore)
        {
            BestTactic = Result;
        }
    }

    return BestTactic;
}

// 识别卧槽马战术（马到对方2·3/2·7位置，配合将军/抽车）
FTacticEvalResult UAI2P::RecognizeWoCaoMa(EChessColor AiColor)
{
    FTacticEvalResult Result;
    Result.TacticType = EChessTactic::WoCaoMa;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 OppKingX, OppKingY;
    if (!GetKingPosition(OpponentColor, OppKingX, OppKingY)) return Result;

    // 卧槽马目标位置（红方：X=2,Y=3/7；黑方：X=7,Y=3/7）
    TArray<FIntPoint> WoCaoPositions;
    if (AiColor == EChessColor::REDCHESS)
    {
        WoCaoPositions = { FIntPoint(2, 3), FIntPoint(2, 7) };
    }
    else
    {
        WoCaoPositions = { FIntPoint(7, 3), FIntPoint(7, 7) };
    }

    // 遍历己方马，判断是否能走到卧槽位
    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::MA) continue;

            // 生成马的所有合法走法
            TArray<FChessMove2P> HorseMoves = GenerateMovesForChess(i, j, Chess);

            for (const FChessMove2P& Move : HorseMoves)
            {
                // 判断是否走到卧槽位
                bool bIsWoCao = false;
                for (const FIntPoint& Pos : WoCaoPositions)
                {
                    if (Move.to.X == Pos.X && Move.to.Y == Pos.Y)
                    {
                        bIsWoCao = true;
                        break;
                    }
                }
                if (!bIsWoCao) continue;

                // 评估可行性：1. 走法安全 2. 能将军/牵制对方将
                int32 Feasibility = 0;
                if (IsCaptureSafe(Move, AiColor)) Feasibility += 50;

                // 模拟走法，判断是否将军/限制对方将移动
                TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
                TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
                MakeTestMove(Move, Moved);
                bool bIsCheck = IsInCheck(OpponentColor);
                // 计算对方将的可移动位置数量（越少可行性越高）
                int32 KingMovableCount = 0;
                TArray<FIntPoint> KingMoves = {
                    FIntPoint(OppKingX - 1, OppKingY), FIntPoint(OppKingX + 1, OppKingY),
                    FIntPoint(OppKingX, OppKingY - 1), FIntPoint(OppKingX, OppKingY + 1)
                };
                for (const FIntPoint& KM : KingMoves)
                {
                    if (KM.X < 0 || KM.X >= 10 || KM.Y < 0 || KM.Y >= 9) continue;
                    if (!GetChess(KM.X, KM.Y).IsValid()) KingMovableCount++;
                }
                UndoTestMove(Move, Moved, Captured);

                if (bIsCheck) Feasibility += 40;
                Feasibility += (4 - KingMovableCount) * 5; // 限制越多，分值越高

                // 更新最优战术走法
                if (Feasibility > MaxFeasibility)
                {
                    // 校验战术走法是否自将，若是则跳过
                    if (!IsInCheckAfterMove(Move, AiColor))
                    {
                        MaxFeasibility = Feasibility;
                        BestMove = Move;
                    }
                }
            }
        }
    }

    Result.FeasibilityScore = MaxFeasibility;
    Result.BestTacticMove = BestMove;
    return Result;
}

// 识别沉底炮战术（炮沉对方底线，配合车/兵进攻）
FTacticEvalResult UAI2P::RecognizeChenDiPao(EChessColor AiColor)
{
    FTacticEvalResult Result;
    Result.TacticType = EChessTactic::ChenDiPao;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    // 沉底炮目标位置（红方：X=0；黑方：X=9）
    int32 DiPaoX = (OpponentColor == EChessColor::REDCHESS) ? 0 : 9;

    // 遍历己方炮，判断是否能沉底
    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::PAO) continue;

            // 生成炮的所有合法走法
            TArray<FChessMove2P> PaoMoves = GenerateMovesForChess(i, j, Chess);
            TArray<FChessMove2P> PaoValidMoves = FilterInvalidMoves(PaoMoves, AiColor, EChessType::PAO);
            if (PaoValidMoves.Num() == 0) continue;

            for (const FChessMove2P& Move : PaoValidMoves)
            {
                // 判断是否沉底
                if (Move.to.X != DiPaoX) continue;

                // 评估可行性：1. 安全 2. 有车/兵配合 3. 能威胁对方将/核心棋子
                int32 Feasibility = 0;
                if (IsCaptureSafe(Move, AiColor)) Feasibility += 40;
                else Feasibility -= 80;

                // 模拟走法，检查是否有车/兵配合
                TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
                TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
                MakeTestMove(Move, Moved);
                // 检查同列是否有己方车/兵
                bool bHasChe = false;
                for (int32 k = 0; k < 10; k++)
                {
                    if (k == DiPaoX) continue;
                    TWeakObjectPtr<AChesses> C = GetChess(k, Move.to.Y);
                    if (C.IsValid() && C->GetColor() == AiColor && (C->GetType() == EChessType::JV || C->GetType() == EChessType::BING))
                    {
                        bHasChe = true;
                        break;
                    }
                }
                if (bHasChe) Feasibility += 40;
                else Feasibility -= 10;

                // 检查是否能威胁对方将/士/象
                bool bThreatKing = false;
                int32 OppKingX, OppKingY;
                if (GetKingPosition(OpponentColor, OppKingX, OppKingY))
                {
                    if (OppKingY == Move.to.Y) bThreatKing = true;
                }
                if (bThreatKing) Feasibility += 20;

                UndoTestMove(Move, Moved, Captured);

                if (Feasibility > MaxFeasibility)
                {
                    // 校验战术走法是否自将，若是则跳过
                    if (!IsInCheckAfterMove(Move, AiColor))
                    {
                        MaxFeasibility = Feasibility;
                        BestMove = Move;
                    }
                }
            }
        }
    }

    Result.FeasibilityScore = MaxFeasibility;
    Result.BestTacticMove = BestMove;
    return Result;
}

// 识别中路突破战术（控制中路，用炮/车推进，压制对方将）
FTacticEvalResult UAI2P::RecognizeZhongLuTuPo(EChessColor AiColor)
{
    FTacticEvalResult Result;
    Result.TacticType = EChessTactic::ZhongLuTuPo;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 OppKingX, OppKingY;
    if (!GetKingPosition(OpponentColor, OppKingX, OppKingY)) return Result;

    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    // === 更灵活的中路进攻识别 ===

    // 不仅限于车和炮，也包括马和过河兵
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor) continue;

            EChessType Type = Chess->GetType();

            // 扩展可执行中路突破的棋子类型
            if (Type != EChessType::JV && Type != EChessType::PAO &&
                Type != EChessType::MA && Type != EChessType::BING)
                continue;

            TArray<FChessMove2P> PieceMoves = GenerateMovesForChess(i, j, Chess);
            TArray<FChessMove2P> ValidMoves = FilterInvalidMoves(PieceMoves, AiColor, Type);

            for (const FChessMove2P& Move : ValidMoves)
            {
                int32 Feasibility = 0;

                // 1. 安全基础分
                if (IsCaptureSafe(Move, AiColor)) Feasibility += 30;

                // 2. 中路控制奖励（不只是Y=4，包括整个中路区域）
                if (Move.to.Y >= 3 && Move.to.Y <= 5)
                {
                    Feasibility += 50;

                    // 特别奖励靠近对方将帅的中路位置
                    if (FMath::Abs(Move.to.X - OppKingX) <= 2)
                    {
                        Feasibility += 80;
                    }
                }

                // 3. 进攻协同奖励
                Feasibility += EvaluateAttackSynergy(Move, AiColor) / 10;

                // 4. 兵种特定奖励
                switch (Type)
                {
                case EChessType::BING:
                    // 过河兵推进到中路奖励
                    if ((AiColor == EChessColor::REDCHESS && Move.to.X < 5) ||
                        (AiColor == EChessColor::BLACKCHESS && Move.to.X > 4))
                    {
                        Feasibility += 60;
                    }
                    break;
                case EChessType::MA:
                    // 马跳到中路关键点奖励
                    if ((Move.to.X == 2 || Move.to.X == 7) && (Move.to.Y == 3 || Move.to.Y == 5))
                    {
                        Feasibility += 70;
                    }
                    break;
                }

                if (Feasibility > MaxFeasibility && !IsInCheckAfterMove(Move, AiColor))
                {
                    MaxFeasibility = Feasibility;
                    BestMove = Move;
                }
            }
        }
    }

    Result.FeasibilityScore = MaxFeasibility;
    Result.BestTacticMove = BestMove;
    return Result;
}

// 识别双车错战术（双车配合，交替将军取胜）
FTacticEvalResult UAI2P::RecognizeShuangCheCuo(EChessColor AiColor)
{
    FTacticEvalResult Result;
    Result.TacticType = EChessTactic::ShuangCheCuo;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 OppKingX, OppKingY;
    if (!GetKingPosition(OpponentColor, OppKingX, OppKingY)) return Result;

    // 先检查是否有至少两辆己方车
    TArray<TWeakObjectPtr<AChesses>> MyChes;
    TArray<std::pair<int32, int32>> ChesLoc;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::JV)
            {
                MyChes.Add(Chess);
                ChesLoc.Add({ i,j });
            }
        }
    }
    if (MyChes.Num() < 2) return Result; // 少于两车，无法执行

    // 评估双车配合的可行性
    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 c1 = 0; c1 < MyChes.Num(); c1++)
    {
        // 【修改1】生成车的原始移动 → 过滤无效移动
        TArray<FChessMove2P> Che1RawMoves = GenerateMovesForChess(ChesLoc[c1].first, ChesLoc[c1].second, MyChes[c1]);
        TArray<FChessMove2P> Che1ValidMoves = FilterInvalidMoves(Che1RawMoves, AiColor, EChessType::JV);
        if (Che1ValidMoves.Num() == 0) continue; // 无有效移动，跳过

        for (const FChessMove2P& Move1 : Che1ValidMoves)
        {
            // 模拟第一辆车走法
            TWeakObjectPtr<AChesses> Moved1 = GetChess(Move1.from.X, Move1.from.Y);
            TWeakObjectPtr<AChesses> Captured1 = GetChess(Move1.to.X, Move1.to.Y);
            MakeTestMove(Move1, Moved1);

            // 检查第二辆车是否能将军
            bool bHasSecondCheck = false;
            FChessMove2P SecondMove;
            for (int32 c2 = 0; c2 < MyChes.Num(); c2++)
            {
                if (c1 == c2) continue;

                // 【修改2】第二辆车也过滤无效移动
                TArray<FChessMove2P> Che2RawMoves = GenerateMovesForChess(ChesLoc[c2].first, ChesLoc[c2].second, MyChes[c2]);
                TArray<FChessMove2P> Che2ValidMoves = FilterInvalidMoves(Che2RawMoves, AiColor, EChessType::JV);
                if (Che2ValidMoves.Num() == 0) continue;

                for (const FChessMove2P& Move2 : Che2ValidMoves)
                {
                    TWeakObjectPtr<AChesses> Moved2 = GetChess(Move2.from.X, Move2.from.Y);
                    TWeakObjectPtr<AChesses> Captured2 = GetChess(Move2.to.X, Move2.to.Y);
                    MakeTestMove(Move2, Moved2);
                    bool bIsCheck = IsInCheck(OpponentColor);
                    UndoTestMove(Move2, Moved2, Captured2);

                    if (bIsCheck)
                    {
                        bHasSecondCheck = true;
                        SecondMove = Move1;
                        break;
                    }
                }
                if (bHasSecondCheck) break;
            }
            UndoTestMove(Move1, Moved1, Captured1);

            // 计算可行性
            int32 Feasibility = 0;
            if (IsCaptureSafe(Move1, AiColor)) Feasibility += 40;
            if (bHasSecondCheck) Feasibility += 50;

            if (Feasibility > MaxFeasibility)
            {
                // 校验战术走法是否自将，若是则跳过
                if (!IsInCheckAfterMove(SecondMove, AiColor))
                {
                    MaxFeasibility = Feasibility;
                    BestMove = SecondMove;
                }
            }

        }
    }

    Result.FeasibilityScore = MaxFeasibility;
    Result.BestTacticMove = BestMove;
    return Result;
}

// 识别兵线推进战术（残局兵过河后的推进与牵制）
FTacticEvalResult UAI2P::RecognizeBingXianTuiJin(EChessColor AiColor)
{
    FTacticEvalResult Result;
    Result.TacticType = EChessTactic::BingXianTuiJin;

    if (GamePhase != EChessGamePhase::Endgame) return Result; // 仅残局执行

    // 遍历己方过河兵，优先推进
    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::BING) continue;

            // 判断是否过河
            bool bCrossed = (AiColor == EChessColor::REDCHESS) ? (i < 5) : (i >= 5);
            if (!bCrossed) continue;

            // 生成兵的合法走法（优先向前推进）
            TArray<FChessMove2P> BingMoves = GenerateMovesForChess(i, j, Chess);
            for (const FChessMove2P& Move : BingMoves)
            {
                // 兵向前推进（红方X减小，黑方X增大）
                bool bIsForward = (AiColor == EChessColor::REDCHESS) ? (Move.to.X < i) : (Move.to.X > i);
                if (!bIsForward) continue;

                // 评估可行性：1. 安全 2. 靠近对方将 3. 能牵制
                int32 Feasibility = 0;
                if (IsCaptureSafe(Move, AiColor)) Feasibility += 50;

                // 计算到对方将的距离（越近分值越高）
                int32 OppKingX, OppKingY;
                if (GetKingPosition((AiColor == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS, OppKingX, OppKingY))
                {
                    int32 Distance = FMath::Abs(Move.to.X - OppKingX) + FMath::Abs(Move.to.Y - OppKingY);
                    Feasibility += (10 - Distance) * 10;
                }

                if (Feasibility > MaxFeasibility)
                {
                    // 校验战术走法是否自将，若是则跳过
                    if (!IsInCheckAfterMove(Move, AiColor))
                    {
                        MaxFeasibility = Feasibility;
                        BestMove = Move;
                    }
                }
            }
        }
    }

    Result.FeasibilityScore = MaxFeasibility;
    Result.BestTacticMove = BestMove;
    return Result;
}

// ===================== 防守核心逻辑 =====================
// 预判对方是否有致命进攻（直接威胁将/帅）
bool UAI2P::IsOpponentHasLethalAttack(EChessColor AiColor)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    // 生成对方所有合法走法，判断是否能直接吃将/帅
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& Move : OpponentMoves)
    {
        if (Move.to.X == KingX && Move.to.Y == KingY)
        {
            return true;
        }
    }
    return false;
}

// 评估防守弱点（值越低弱点越大：空门/关键点位无保护）
int32 UAI2P::EvaluateDefenseWeakness(EChessColor AiColor)
{
    int32 WeaknessScore = 0;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return 0;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    if (AIDifficulty == EAI2PDifficulty::Hard)
    {
        // 检测双炮将军威胁
        TArray<FIntPoint> OpponentPaos;
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::PAO)
                {
                    OpponentPaos.Add(FIntPoint(i, j));
                }
            }
        }

        // 检测双炮同线威胁
        if (OpponentPaos.Num() >= 2)
        {
            for (int32 i = 0; i < OpponentPaos.Num(); i++)
            {
                for (int32 j = i + 1; j < OpponentPaos.Num(); j++)
                {
                    FIntPoint Pao1 = OpponentPaos[i];
                    FIntPoint Pao2 = OpponentPaos[j];

                    // 检查是否在同一条直线（横线或竖线）
                    bool bSameLine = (Pao1.X == Pao2.X) || (Pao1.Y == Pao2.Y);
                    if (bSameLine)
                    {
                        // 检查是否与将帅在同一条直线
                        bool bThreatKing = false;
                        if (Pao1.X == Pao2.X && Pao1.X == KingX)
                        {
                            // 横线威胁：将帅在同一横线
                            bThreatKing = true;
                        }
                        else if (Pao1.Y == Pao2.Y && Pao1.Y == KingY)
                        {
                            // 竖线威胁：将帅在同一竖线
                            bThreatKing = true;
                        }

                        if (bThreatKing)
                        {
                            // 检查中间棋子数量（重炮需要恰好1个炮架）
                            int32 PieceCount = 0;
                            if (Pao1.X == Pao2.X) // 横线
                            {
                                int32 MinY = FMath::Min(Pao1.Y, Pao2.Y);
                                int32 MaxY = FMath::Max(Pao1.Y, Pao2.Y);
                                for (int32 y = MinY + 1; y < MaxY; y++)
                                {
                                    if (GetChess(Pao1.X, y).IsValid()) PieceCount++;
                                }
                            }
                            else // 竖线
                            {
                                int32 MinX = FMath::Min(Pao1.X, Pao2.X);
                                int32 MaxX = FMath::Max(Pao1.X, Pao2.X);
                                for (int32 x = MinX + 1; x < MaxX; x++)
                                {
                                    if (GetChess(x, Pao1.Y).IsValid()) PieceCount++;
                                }
                            }

                            // 双炮威胁评估：中间有1个棋子时威胁最大
                            if (PieceCount == 1)
                            {
                                WeaknessScore -= 1500; // 重炮威胁，大幅扣分
                            }
                            else if (PieceCount == 0)
                            {
                                WeaknessScore -= 800; // 潜在重炮威胁
                            }
                        }
                    }
                }
            }
        }
    }

    // 1. 检查将门是否有保护（增强对关键位置的防守）
    TArray<FIntPoint> KingGatePoints = {
        FIntPoint(KingX, KingY),
        FIntPoint(KingX, KingY - 1),
        FIntPoint(KingX, KingY + 1),
        FIntPoint(KingX - 1, KingY),  // 新增：两侧保护
        FIntPoint(KingX + 1, KingY)   // 新增：两侧保护
    };

    for (const FIntPoint& Point : KingGatePoints)
    {
        if (Point.X < 0 || Point.X >= 10 || Point.Y < 0 || Point.Y >= 9) continue;

        if (!IsPieceRooted(Point.X, Point.Y, AiColor))
        {
            // 根据位置重要性差异化扣分
            if (Point.X == KingX && (Point.Y == KingY - 1 || Point.Y == KingY + 1))
                WeaknessScore -= 400; // 将门正前方/后方无保护
            else if (Point.X == KingX && Point.Y == KingY)
                WeaknessScore -= 600; // 将帅本身无保护
            else
                WeaknessScore -= 200; // 两侧无保护
        }
    }

    // 2. 检查中路（Y=4）是否有保护（适度增强）
    for (int32 i = (AiColor == EChessColor::REDCHESS ? 0 : 7); i <= (AiColor == EChessColor::REDCHESS ? 2 : 9); i++)
    {
        if (!IsPieceRooted(i, 4, AiColor))
        {
            WeaknessScore -= 120; // 适度降低中路扣分，避免过度防守
        }
    }

    // 3. 检查核心棋子是否被攻击（保持原有逻辑）
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor) continue;

            EChessType Type = Chess->GetType();
            if (Type == EChessType::JV || Type == EChessType::MA || Type == EChessType::PAO)
            {
                if (!IsPieceRooted(i, j, AiColor) && IsPieceIsolated(i, j, AiColor))
                {
                    WeaknessScore -= 250; // 适度降低扣分，保持进攻性
                }
            }
        }
    }

    return WeaknessScore;
}

// 检查是否存在双炮威胁
bool UAI2P::HasDoublePaoThreat(EChessColor AiColor)
{
    if ((int32)AIDifficulty < 1) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    TArray<FIntPoint> OpponentPaos;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::PAO)
            {
                OpponentPaos.Add(FIntPoint(i, j));
            }
        }
    }

    if (OpponentPaos.Num() < 2) return false;

    // 检查双炮威胁逻辑（与EvaluateDefenseWeakness中类似）
    for (int32 i = 0; i < OpponentPaos.Num(); i++)
    {
        for (int32 j = i + 1; j < OpponentPaos.Num(); j++)
        {
            FIntPoint Pao1 = OpponentPaos[i];
            FIntPoint Pao2 = OpponentPaos[j];

            if ((Pao1.X == Pao2.X && Pao1.X == KingX) ||
                (Pao1.Y == Pao2.Y && Pao1.Y == KingY))
            {
                return true; // 存在双炮威胁
            }
        }
    }

    return false;
}

// 专门评估双炮威胁
int32 UAI2P::EvaluateDoublePaoThreat(EChessColor AiColor)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return 0;

    int32 ThreatScore = 0;

    // 收集对方所有炮的位置
    TArray<FIntPoint> OpponentCannons;
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::PAO) {
                OpponentCannons.Add(FIntPoint(i, j));
            }
        }
    }

    // 单个炮威胁评估
    for (const FIntPoint& Cannon : OpponentCannons) {
        bool sameLine = (Cannon.X == KingX) || (Cannon.Y == KingY);
        if (!sameLine) continue;

        // 检查中间棋子数量
        int32 pieceCount = 0;
        if (Cannon.X == KingX) {
            int32 startY = FMath::Min(Cannon.Y, KingY) + 1;
            int32 endY = FMath::Max(Cannon.Y, KingY) - 1;
            for (int32 y = startY; y <= endY; y++) {
                if (GetChess(KingX, y).IsValid()) pieceCount++;
            }
        }
        else {
            int32 startX = FMath::Min(Cannon.X, KingX) + 1;
            int32 endX = FMath::Max(Cannon.X, KingX) - 1;
            for (int32 x = startX; x <= endX; x++) {
                if (GetChess(x, KingY).IsValid()) pieceCount++;
            }
        }

        // 炮威胁惩罚大幅提升
        if (pieceCount == 1) {
            ThreatScore -= 1200; // 从800提升到1200
        }
        else if (pieceCount == 0) {
            ThreatScore -= 500; // 从300提升到500
        }
    }

    // 双炮协同威胁评估（严重威胁）
    if (OpponentCannons.Num() >= 2) {
        for (int32 i = 0; i < OpponentCannons.Num(); i++) {
            for (int32 j = i + 1; j < OpponentCannons.Num(); j++) {
                FIntPoint cannon1 = OpponentCannons[i];
                FIntPoint cannon2 = OpponentCannons[j];

                bool bothThreatenKing = false;
                if (cannon1.X == KingX && cannon2.X == KingX) {
                    bothThreatenKing = true;
                }
                else if (cannon1.Y == KingY && cannon2.Y == KingY) {
                    bothThreatenKing = true;
                }

                if (bothThreatenKing) {
                    ThreatScore -= 3000; // 从2000提升到3000

                    // 立即双炮杀检测
                    if (IsImmediateDoublePaoMate(AiColor)) {
                        return -10000; // 极低分数
                    }
                }
            }
        }
    }

    return ThreatScore;
}

// 检查是否立即的双炮杀
bool UAI2P::IsImmediateDoublePaoMate(EChessColor AiColor)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 生成对方所有走法
    TArray<FChessMove2P> opponentMoves = GenerateAllMoves(OpponentColor);

    for (const FChessMove2P& move : opponentMoves) {
        // 模拟走法
        TWeakObjectPtr<AChesses> moved = GetChess(move.from.X, move.from.Y);
        TWeakObjectPtr<AChesses> captured = GetChess(move.to.X, move.to.Y);
        MakeTestMove(move, moved);

        // 检查是否形成双炮将军
        bool isDoubleCannonCheck = HasDoublePaoThreat(AiColor);

        UndoTestMove(move, moved, captured);

        if (isDoubleCannonCheck) {
            // 再检查将帅是否无法移动（被将死）
            bool canEscape = CanKingEscapeDoublePao(AiColor, move);
            if (!canEscape) {
                return true; // 立即的双炮杀
            }
        }
    }

    return false;
}

bool UAI2P::CanKingEscapeDoublePao(EChessColor AiColor, const FChessMove2P& opponentMove)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟对方的走法
    TWeakObjectPtr<AChesses> movedByOpponent = GetChess(opponentMove.from.X, opponentMove.from.Y);
    TWeakObjectPtr<AChesses> capturedByOpponent = GetChess(opponentMove.to.X, opponentMove.to.Y);
    MakeTestMove(opponentMove, movedByOpponent);

    // 获取将帅当前位置
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY))
    {
        UndoTestMove(opponentMove , movedByOpponent, capturedByOpponent);
        return false;
    }

    // 生成将帅所有可能的移动
    TArray<FChessMove2P> kingMoves;
    TWeakObjectPtr<AChesses> king = GetChess(KingX, KingY);
    if (king.IsValid())
    {
        kingMoves = GenerateMovesForChess(KingX, KingY, king);
    }

    // 过滤掉会导致自将的移动
    TArray<FChessMove2P> validKingMoves;
    for (const FChessMove2P& kingMove : kingMoves)
    {
        if (!IsInCheckAfterMove(kingMove, AiColor))
        {
            validKingMoves.Add(kingMove);
        }
    }

    // 检查每个可能的移动是否能逃脱将军
    bool canEscape = false;
    for (const FChessMove2P& escapeMove : validKingMoves)
    {
        // 模拟将帅移动
        TWeakObjectPtr<AChesses> movedByKing = GetChess(escapeMove.from.X, escapeMove.from.Y);
        TWeakObjectPtr<AChesses> capturedByKing = GetChess(escapeMove.to.X, escapeMove.to.Y);
        MakeTestMove(escapeMove, movedByKing);

        // 检查移动后是否仍然被将军
        bool stillInCheck = IsInCheck(AiColor);

        // 撤销将帅移动
        UndoTestMove(escapeMove, movedByKing, capturedByKing);

        if (!stillInCheck)
        {
            canEscape = true;
            break;
        }
    }

    // 检查是否可以通过吃炮解除将军
    if (!canEscape)
    {
        // 找出将军的炮
        TArray<FIntPoint> threateningCannons;
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> chess = GetChess(i, j);
                if (chess.IsValid() && chess->GetColor() == OpponentColor && chess->GetType() == EChessType::PAO)
                {
                    // 检查这个炮是否能将军
                    TArray<FChessMove2P> cannonMoves = GenerateMovesForChess(i, j, chess);
                    for (const FChessMove2P& cannonMove : cannonMoves)
                    {
                        if (cannonMove.to.X == KingX && cannonMove.to.Y == KingY)
                        {
                            threateningCannons.Add(FIntPoint(i, j));
                            break;
                        }
                    }
                }
            }
        }

        // 检查是否能吃掉这些炮
        for (const FIntPoint& cannonPos : threateningCannons)
        {
            // 生成所有能吃这个炮的走法
            for (int32 i = 0; i < 10; i++)
            {
                for (int32 j = 0; j < 9; j++)
                {
                    TWeakObjectPtr<AChesses> attacker = GetChess(i, j);
                    if (attacker.IsValid() && attacker->GetColor() == AiColor)
                    {
                        TArray<FChessMove2P> attackMoves = GenerateMovesForChess(i, j, attacker);
                        for (const FChessMove2P& attackMove : attackMoves)
                        {
                            if (attackMove.to.X == cannonPos.X && attackMove.to.Y == cannonPos.Y)
                            {
                                // 检查吃炮是否安全（不会导致自将）
                                if (!IsInCheckAfterMove(attackMove, AiColor))
                                {
                                    canEscape = true;
                                    break;
                                }
                            }
                        }
                        if (canEscape) break;
                    }
                }
                if (canEscape) break;
            }
            if (canEscape) break;
        }
    }

    // 检查是否可以通过垫将解除将军
    if (!canEscape)
    {
        // 找出将军的炮的路径
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> cannon = GetChess(i, j);
                if (cannon.IsValid() && cannon->GetColor() == OpponentColor && cannon->GetType() == EChessType::PAO)
                {
                    // 检查这个炮是否能将军
                    bool canCheck = false;
                    TArray<FChessMove2P> cannonMoves = GenerateMovesForChess(i, j, cannon);
                    for (const FChessMove2P& cannonMove : cannonMoves)
                    {
                        if (cannonMove.to.X == KingX && cannonMove.to.Y == KingY)
                        {
                            canCheck = true;
                            break;
                        }
                    }

                    if (canCheck)
                    {
                        // 找出炮和将之间的所有位置
                        TArray<FIntPoint> blockingPositions;
                        if (i == KingX) // 同一横线
                        {
                            int32 startY = FMath::Min(j, KingY) + 1;
                            int32 endY = FMath::Max(j, KingY) - 1;
                            for (int32 y = startY; y <= endY; y++)
                            {
                                blockingPositions.Add(FIntPoint(i, y));
                            }
                        }
                        else if (j == KingY) // 同一竖线
                        {
                            int32 startX = FMath::Min(i, KingX) + 1;
                            int32 endX = FMath::Max(i, KingX) - 1;
                            for (int32 x = startX; x <= endX; x++)
                            {
                                blockingPositions.Add(FIntPoint(x, j));
                            }
                        }

                        // 检查是否能移动到这些位置阻挡炮线
                        for (const FIntPoint& blockPos : blockingPositions)
                        {
                            // 生成所有能走到这个位置的走法
                            for (int32 x = 0; x < 10; x++)
                            {
                                for (int32 y = 0; y < 9; y++)
                                {
                                    TWeakObjectPtr<AChesses> blocker = GetChess(x, y);
                                    if (blocker.IsValid() && blocker->GetColor() == AiColor && blocker->GetType() != EChessType::JIANG)
                                    {
                                        TArray<FChessMove2P> blockMoves = GenerateMovesForChess(x, y, blocker);
                                        for (const FChessMove2P& blockMove : blockMoves)
                                        {
                                            if (blockMove.to.X == blockPos.X && blockMove.to.Y == blockPos.Y)
                                            {
                                                // 检查阻挡是否安全（不会导致自将）
                                                if (!IsInCheckAfterMove(blockMove, AiColor))
                                                {
                                                    canEscape = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (canEscape) break;
                                    }
                                }
                                if (canEscape) break;
                            }
                            if (canEscape) break;
                        }
                    }
                }
                if (canEscape) break;
            }
            if (canEscape) break;
        }
    }

    // 撤销对方走法
    UndoTestMove(opponentMove, movedByOpponent, capturedByOpponent);

    return canEscape;
}

// 检查走法是否能防御双炮
bool UAI2P::CanDefendAgainstDoublePao(const FChessMove2P& Move, EChessColor AiColor)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟走法
    TWeakObjectPtr<AChesses> moved = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> captured = GetChess(Move.to.X, Move.to.Y);
    MakeTestMove(Move, moved);

    // 检查走法后是否还面临严重的双炮威胁
    int32 threatAfterMove = EvaluateDoublePaoThreat(AiColor);

    UndoTestMove(Move, moved, captured);

    // 如果威胁显著降低，认为这个走法有效
    return (threatAfterMove > -500); // 威胁降低到可接受水平
}

// 检查是否阻挡炮线
bool UAI2P::BlocksPaoLine(const FChessMove2P& Move, EChessColor AiColor)
{
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 检查移动后的位置是否在对方炮和将帅之间的关键位置
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> chess = GetChess(i, j);
            if (chess.IsValid() && chess->GetColor() == OpponentColor && chess->GetType() == EChessType::PAO) {
                // 检查炮是否威胁将帅
                if (i == KingX || j == KingY) {
                    // 检查移动后的位置是否在炮线上
                    if ((i == KingX && Move.to.X == KingX &&
                        ((Move.to.Y > j && Move.to.Y < KingY) || (Move.to.Y < j && Move.to.Y > KingY))) ||
                        (j == KingY && Move.to.Y == KingY &&
                            ((Move.to.X > i && Move.to.X < KingX) || (Move.to.X < i && Move.to.X > KingX)))) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// 检查是否阻挡车线
bool UAI2P::BlocksCheLine(const FChessMove2P& Move, EChessColor Color, TWeakObjectPtr<AChesses> Attacker)
{
    if (!Attacker.IsValid() || Attacker->GetType() != EChessType::JV)
        return false;

    int32 KingX, KingY;
    if (!GetKingPosition(Color, KingX, KingY))
        return false;

    int32 AttackerX = Attacker->GetSimpPosition().X;
    int32 AttackerY = Attacker->GetSimpPosition().Y;

    // 检查移动后的位置是否在车和将的直线上
    if (AttackerX == KingX) // 同一横线
    {
        if (Move.to.X == KingX &&
            ((Move.to.Y > AttackerY && Move.to.Y < KingY) ||
                (Move.to.Y < AttackerY && Move.to.Y > KingY)))
        {
            return true;
        }
    }
    else if (AttackerY == KingY) // 同一竖线
    {
        if (Move.to.Y == KingY &&
            ((Move.to.X > AttackerX && Move.to.X < KingX) ||
                (Move.to.X < AttackerX && Move.to.X > KingX)))
        {
            return true;
        }
    }

    return false;
}

// 判断走法是否为有效防守（拦截致命进攻/补位弱点）
bool UAI2P::IsEffectiveDefenseMove(const FChessMove2P& Move, EChessColor AiColor)
{
    // 优先级1：拦截致命进攻
    if (IsOpponentHasLethalAttack(AiColor))
    {
        // 模拟走法后，检查对方是否还能致命进攻
        TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
        TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
        MakeTestMove(Move, Moved);
        bool bStillHasLethal = IsOpponentHasLethalAttack(AiColor);

        // 检查双炮威胁是否被化解
        bool bDoublePaoThreatRemoved = false;
        if (bStillHasLethal)
        {
            // 检查移动后是否破坏了对方的双炮线路
            bDoublePaoThreatRemoved = !HasDoublePaoThreat(AiColor);
        }

        UndoTestMove(Move, Moved, Captured);

        if (!bStillHasLethal || bDoublePaoThreatRemoved) return true;
    }

    // 优先级2：补位防守弱点（走法后防守弱点分值提升）
    int32 WeaknessBefore = EvaluateDefenseWeakness(AiColor);
    TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
    MakeTestMove(Move, Moved);
    int32 WeaknessAfter = EvaluateDefenseWeakness(AiColor);
    UndoTestMove(Move, Moved, Captured);

    if (WeaknessAfter > WeaknessBefore + 100) return true; // 弱点大幅改善

    // 优先级3：主动防守（卡位，限制对方进攻空间）
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    TArray<FChessMove2P> OpponentMovesBefore = GenerateAllMoves(OpponentColor);
    MakeTestMove(Move, Moved);
    TArray<FChessMove2P> OpponentMovesAfter = GenerateAllMoves(OpponentColor);
    UndoTestMove(Move, Moved, Captured);

    if (OpponentMovesAfter.Num() < OpponentMovesBefore.Num() - 2) return true; // 对方走法减少，卡位有效

    return false;
}

// 评估走法的进攻协同性（多棋子配合度）
int32 UAI2P::EvaluateAttackSynergy(const FChessMove2P& Move, EChessColor AiColor)
{
    // 模拟走法
    TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
    MakeTestMove(Move, Moved);

    int32 SynergyScore = 0;
    TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.to.X, Move.to.Y);
    if (!MovedChess.IsValid())
    {
        UndoTestMove(Move, Moved, Captured);
        return 0;
    }

    EChessType MovedType = MovedChess->GetType();
    int32 X = Move.to.X, Y = Move.to.Y;
    if (GamePhase == EChessGamePhase::Opening)
    {
        SynergyScore += EvaluateCenterProtection(AiColor);
        SynergyScore += EvaluatePieceDevelopmentQuick(AiColor);
        SynergyScore += EvaluateDoublePaoThreat(AiColor);
        if (MovedType == EChessType::MA)
        {
            int32 GoodHorseValue = 0;
            switch (AIDifficulty)
            {
            case EAI2PDifficulty::Easy:
                GoodHorseValue = 120;
                break;
            case EAI2PDifficulty::Normal:
                GoodHorseValue = 200;
                break;
            case EAI2PDifficulty::Hard:
                GoodHorseValue = 300;
                break;
            }
            if (GamePhase == EChessGamePhase::Opening)
            {
                GoodHorseValue += 200;
            }
            SynergyScore += IsGoodHorsePosition(Move.to.X, Move.to.Y, AiColor) ? GoodHorseValue : 0;
        }
    }
    else
    {
        SynergyScore += EvaluateDoublePaoThreat(AiColor);
        SynergyScore += EvaluatePieceDevelopmentQuick(AiColor);

        // 兵种配合
        SynergyScore += EvaluatePieceCombination(X, Y, AiColor);

        // 线路控制协同
        SynergyScore += EvaluateLineControlCooperation(X, Y, AiColor);
    }

    // 撤销走法
    UndoTestMove(Move, Moved, Captured);

    return SynergyScore;
}

// 判断是否控制关键点位
bool UAI2P::IsControlKeyPoint(int32 X, int32 Y, EKeyChessPoint PointType, EChessColor Color)
{
    switch (PointType)
    {
    case EKeyChessPoint::Center:
        return Y == 4 && GetChess(X, Y).IsValid() && GetChess(X, Y)->GetColor() == Color;
    case EKeyChessPoint::KingGate:
    {
        int32 TargetKingX = (Color == EChessColor::REDCHESS) ? 9 : 0;
        return X == TargetKingX && Y == 4 && GetChess(X, Y).IsValid() && GetChess(X, Y)->GetColor() == Color;
    }
    case EKeyChessPoint::SoldierLine:
    {
        int32 TargetX = (Color == EChessColor::REDCHESS) ? 5 : 4;
        return X == TargetX && GetChess(X, Y).IsValid() && GetChess(X, Y)->GetColor() == Color;
    }
    case EKeyChessPoint::HorsePoint:
        return (X == 2 && Y == 1) || (X == 2 && Y == 7) || (X == 7 && Y == 1) || (X == 7 && Y == 7)
            && GetChess(X, Y).IsValid() && GetChess(X, Y)->GetColor() == Color;
    default:
        return false;
    }
}

int32 UAI2P::EvaluateOpeningFeatures(EChessColor AiColor)
{
    int32 Score = 0;

    // 强化中路保护
    Score += EvaluateCenterProtection(AiColor) * 3;

    // 简化棋子出动评估
    Score += EvaluatePieceDevelopmentQuick(AiColor);

    return Score;
}

int32 UAI2P::EvaluateMidEndgameFeatures(EChessColor AiColor)
{
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 Score = 0;

    // === 中局专项评估 ===
    if (GamePhase == EChessGamePhase::Midgame) {
        // 1. 进攻性评估（简化版）
        int32 Aggressiveness = 0;

        // 计算前场棋子数量
        int32 FrontlinePieces = 0;
        for (int32 i = 0; i < 10; i++) {
            for (int32 j = 0; j < 9; j++) {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor) {
                    // 在前场（对方半场）的棋子
                    if ((AiColor == EChessColor::REDCHESS && i <= 4) ||
                        (AiColor == EChessColor::BLACKCHESS && i >= 5)) {
                        FrontlinePieces++;
                        // 根据棋子类型给予进攻奖励
                        switch (Chess->GetType()) {
                        case EChessType::JV: Aggressiveness += 60; break;
                        case EChessType::PAO: Aggressiveness += 50; break;
                        case EChessType::MA: Aggressiveness += 40; break;
                        case EChessType::BING: Aggressiveness += 30; break;
                        }
                    }
                }
            }
        }
        Score += Aggressiveness;

        // 2. 战术机会识别（简化版）
        FTacticEvalResult BestTactic = EvaluateBestTactic(AiColor);
        if (BestTactic.FeasibilityScore > 50) {
            Score += 200; // 基础战术奖励
            // 根据战术类型额外奖励
            switch (BestTactic.TacticType) {
            case EChessTactic::WoCaoMa: Score += 300; break;
            case EChessTactic::ChenDiPao: Score += 250; break;
            case EChessTactic::ZhongLuTuPo: Score += 200; break;
            default: break;
            }
        }

        // 3. 关键棋子安全评估
        int32 SafetyScore = 0;
        for (int32 i = 0; i < 10; i++) {
            for (int32 j = 0; j < 9; j++) {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor) {
                    EChessType Type = Chess->GetType();
                    // 只评估车、马、炮的安全性
                    if (Type == EChessType::JV || Type == EChessType::MA || Type == EChessType::PAO) {
                        if (!IsPieceRooted(i, j, AiColor)) {
                            // 检查是否被攻击
                            bool IsUnderAttack = false;
                            TArray<FChessMove2P> OppMoves = GenerateAllMoves(OpponentColor);
                            for (const FChessMove2P& Move : OppMoves) {
                                if (Move.to.X == i && Move.to.Y == j) {
                                    IsUnderAttack = true;
                                    break;
                                }
                            }
                            if (IsUnderAttack) {
                                SafetyScore -= 80; // 无保护被攻击
                            }
                        }
                        else {
                            SafetyScore += 30; // 有保护
                        }
                    }
                }
            }
        }
        Score += SafetyScore;
    }

    // === 残局专项评估 ===
    else if (GamePhase == EChessGamePhase::Endgame) {
        // 1. 兵的价值大幅提升
        int32 BingScore = 0;
        int32 OppKingX, OppKingY;
        if (GetKingPosition(OpponentColor, OppKingX, OppKingY)) {
            for (int32 i = 0; i < 10; i++) {
                for (int32 j = 0; j < 9; j++) {
                    TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                    if (Chess.IsValid() && Chess->GetColor() == AiColor &&
                        Chess->GetType() == EChessType::BING) {
                        // 过河兵奖励
                        bool bCrossed = (AiColor == EChessColor::REDCHESS) ? (i < 5) : (i > 4);
                        if (bCrossed) {
                            BingScore += 100;

                            // 计算到对方将帅的距离（越近价值越高）
                            int32 Distance = FMath::Abs(i - OppKingX) + FMath::Abs(j - OppKingY);
                            BingScore += (10 - Distance) * 15;

                            // 进入九宫额外奖励
                            bool bInPalace = false;
                            if (AiColor == EChessColor::REDCHESS) {
                                bInPalace = (i >= 7 && i <= 9) && (j >= 3 && j <= 5);
                            }
                            else {
                                bInPalace = (i >= 0 && i <= 2) && (j >= 3 && j <= 5);
                            }
                            if (bInPalace) BingScore += 200;
                        }
                    }
                }
            }
        }
        Score += BingScore;

        // 2. 将帅活动性评估
        int32 KingActivity = 0;
        int32 MyKingX, MyKingY;
        if (GetKingPosition(AiColor, MyKingX, MyKingY)) {
            // 将帅离开原始位置奖励（残局将帅要主动出击）
            int32 OriginalKingX = (AiColor == EChessColor::REDCHESS) ? 9 : 0;
            if (MyKingX != OriginalKingX) {
                KingActivity += 50;
            }

            // 将帅靠近中线奖励
            if (MyKingY == 4) KingActivity += 40;
        }
        Score += KingActivity;

        // 3. 简化版棋子协调性（避免复杂计算）
        int32 Coordination = 0;
        // 检查车和兵的配合
        bool HasChe = false, HasBing = false;
        for (int32 i = 0; i < 10; i++) {
            for (int32 j = 0; j < 9; j++) {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor) {
                    if (Chess->GetType() == EChessType::JV) HasChe = true;
                    if (Chess->GetType() == EChessType::BING) {
                        bool bCrossed = (AiColor == EChessColor::REDCHESS) ? (i < 5) : (i > 4);
                        if (bCrossed) HasBing = true;
                    }
                }
            }
        }
        if (HasChe && HasBing) Coordination += 100; // 车兵配合

        Score += Coordination;
    }

    return Score;
}

// 快速棋子出动评估
int32 UAI2P::EvaluatePieceDevelopmentQuick(EChessColor Color)
{
    if (GamePhase != EChessGamePhase::Opening) return 0;

    int32 Score = 0;
    int32 CenterY = 4;

    // 只评估关键棋子的位置
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != Color) continue;

            EChessType Type = Chess->GetType();

            // 车：控制开放线路
            if (Type == EChessType::JV && j == CenterY) Score += 50;

            // 马：好位置奖励
            if (Type == EChessType::MA && (i == 2 || i == 7) && (j == 2 || j == 6))
                Score += 40;

            // 炮：中路控制
            if (Type == EChessType::PAO && j == CenterY) Score += 30;
        }
    }

    return Score;
}

// 评估中路保护（开局阶段特别重要）
int32 UAI2P::EvaluateCenterProtection(EChessColor AiColor)
{
    int32 protectionScore = 0;
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 中路位置（Y=4）
    int32 centerY = 4;

    // 检查中路兵的保护情况
    for (int32 x = 0; x < 10; x++)
    {
        TWeakObjectPtr<AChesses> chess = GetChess(x, centerY);
        if (chess.IsValid() && chess->GetColor() == AiColor && chess->GetType() == EChessType::BING)
        {
            // 中路兵的基础保护奖励

            // 检查是否有保护
            if (IsPieceRooted(x, centerY, AiColor))
            {
                protectionScore += 1000; // 有保护的中路兵
            }
            else
            {
                protectionScore -= 1000; // 无保护的中路兵容易被吃
            }
        }
    }

    // 检查对中路控制的奖励
    int32 centerControl = EvaluateCenterControl(AiColor);
    protectionScore += centerControl;

    return protectionScore;
}

// 评估中路控制权
int32 UAI2P::EvaluateCenterControl(EChessColor AiColor)
{
    int32 controlScore = 0;
    int32 centerY = 4;

    // 检查中路关键点位的控制
    TArray<FIntPoint> centerPoints = {
        FIntPoint(3, 4), FIntPoint(4, 4), FIntPoint(5, 4), FIntPoint(6, 4) // 中路核心区域
    };

    for (const FIntPoint& point : centerPoints)
    {
        // 检查是否有己方棋子控制这个点
        bool controlledByUs = false;
        for (int32 x = 0; x < 10; x++)
        {
            for (int32 y = 0; y < 9; y++)
            {
                TWeakObjectPtr<AChesses> chess = GetChess(x, y);
                if (chess.IsValid() && chess->GetColor() == AiColor)
                {
                    // 检查这个棋子是否能攻击或走到中路点位
                    TArray<FChessMove2P> moves = GenerateMovesForChess(x, y, chess);
                    for (const FChessMove2P& move : moves)
                    {
                        if (move.to.X == point.X && move.to.Y == point.Y)
                        {
                            controlledByUs = true;
                            break;
                        }
                    }
                    if (controlledByUs) break;
                }
            }
            if (controlledByUs) break;
        }

        if (controlledByUs)
        {
            controlScore += 60; // 控制中路关键点
        }
    }

    return controlScore;
}

// 评估兵种组合配合
int32 UAI2P::EvaluatePieceCombination(int32 X, int32 Y, EChessColor Color)
{
    int32 combinationScore = 0;

    // 检查周围棋子的兵种组合
    TArray<EChessType> nearbyPieceTypes;

    // 检查3x3范围内的棋子
    for (int32 i = FMath::Max(0, X - 1); i <= FMath::Min(9, X + 1); i++)
    {
        for (int32 j = FMath::Max(0, Y - 1); j <= FMath::Min(8, Y + 1); j++)
        {
            if (i == X && j == Y) continue;

            TWeakObjectPtr<AChesses> chess = GetChess(i, j);
            if (chess.IsValid() && chess->GetColor() == Color)
            {
                nearbyPieceTypes.Add(chess->GetType());
            }
        }
    }

    // 评估兵种组合的价值
    bool hasChe = nearbyPieceTypes.Contains(EChessType::JV);
    bool hasMa = nearbyPieceTypes.Contains(EChessType::MA);
    bool hasPao = nearbyPieceTypes.Contains(EChessType::PAO);
    bool hasBing = nearbyPieceTypes.Contains(EChessType::BING);

    // 经典组合奖励
    if (hasChe && hasMa) combinationScore += 80;  // 车马组合
    if (hasChe && hasPao) combinationScore += 90; // 车炮组合
    if (hasMa && hasPao) combinationScore += 30;  // 马炮组合
    if (hasChe && hasMa && hasPao) combinationScore += 150; // 车马炮组合

    // 兵与其他棋子的配合
    if (hasBing && (hasChe || hasMa || hasPao))
    {
        combinationScore += 60; // 兵种配合
    }

    return combinationScore;
}

// 评估线路控制协同
int32 UAI2P::EvaluateLineControlCooperation(int32 X, int32 Y, EChessColor Color)
{
    int32 lineControlScore = 0;

    // 检查横向和纵向的线路控制
    int32 horizontalControl = 0;
    int32 verticalControl = 0;

    // 横向控制（Y轴相同）
    for (int32 i = 0; i < 10; i++)
    {
        if (i == X) continue;
        TWeakObjectPtr<AChesses> chess = GetChess(i, Y);
        if (chess.IsValid() && chess->GetColor() == Color)
        {
            // 检查是否是控制性棋子（车、炮）
            if (chess->GetType() == EChessType::JV || chess->GetType() == EChessType::PAO)
            {
                horizontalControl++;
            }
        }
    }

    // 纵向控制（X轴相同）
    for (int32 j = 0; j < 9; j++)
    {
        if (j == Y) continue;
        TWeakObjectPtr<AChesses> chess = GetChess(X, j);
        if (chess.IsValid() && chess->GetColor() == Color)
        {
            if (chess->GetType() == EChessType::JV || chess->GetType() == EChessType::PAO)
            {
                verticalControl++;
            }
        }
    }

    // 线路控制奖励
    if (horizontalControl >= 1) lineControlScore += 50;
    if (verticalControl >= 1) lineControlScore += 50;
    if (horizontalControl >= 2) lineControlScore += 80; // 双重控制
    if (verticalControl >= 2) lineControlScore += 80;

    return lineControlScore;
}

// 评估将帅移动到指定位置后的安全程度
int32 UAI2P::EvaluateKingSafety(int32 TargetX, int32 TargetY, EChessColor Color)
{
    int32 SafetyScore = 0;
    EChessColor OppColor = (Color == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;

    // 1. 模拟将帅移动到目标位置
    int32 OriginalKingX, OriginalKingY;
    if (!GetKingPosition(Color, OriginalKingX, OriginalKingY)) return 0;

    // 临时移动将帅进行评估
    TWeakObjectPtr<AChesses> OriginalKing = GetChess(OriginalKingX, OriginalKingY);
    TWeakObjectPtr<AChesses> OriginalTarget = GetChess(TargetX, TargetY);
    FChessMove2P TempMove;
    TempMove.from = FIntPoint(OriginalKingX, OriginalKingY);
    TempMove.to = FIntPoint(TargetX, TargetY);

    MakeTestMove(TempMove, OriginalKing);

    // 2. 评估安全指标
    SafetyScore += 200; // 基础安全分

    // 2.1 检查是否仍被将军
    if (IsInCheck(Color))
    {
        SafetyScore -= 1000; // 移动后仍被将军
    }

    // 2.2 计算对方能攻击到该位置的棋子数量
    int32 AttackersCount = 0;
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OppColor);
    for (const FChessMove2P& Move : OpponentMoves)
    {
        if (Move.to.X == TargetX && Move.to.Y == TargetY)
        {
            AttackersCount++;
        }
    }
    SafetyScore -= AttackersCount * 150; // 每个攻击者减分

    // 2.3 评估位置隐蔽性（远离开放线路）
    if (TargetY != 4) SafetyScore += 50; // 不在中线更安全
    if (TargetX == (Color == EChessColor::REDCHESS ? 9 : 0))
        SafetyScore += 30; // 底线相对安全

    // 2.4 检查是否有己方棋子保护
    if (IsPieceRooted(TargetX, TargetY, Color))
        SafetyScore += 100;

    // 撤销临时移动
    UndoTestMove(TempMove, OriginalKing, OriginalTarget);

    return SafetyScore;
}

// 判断棋子是否有根（有己方保护）
bool UAI2P::IsPieceRooted(int32 X, int32 Y, EChessColor Color)
{
    TWeakObjectPtr<AChesses> TargetChess = GetChess(X, Y);
    if (!TargetChess.IsValid() || TargetChess->GetColor() != Color) return false;

    // 遍历己方所有棋子，判断是否能保护该位置
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> ProtectChess = GetChess(i, j);
            if (!ProtectChess.IsValid() || ProtectChess->GetColor() != Color) continue;

            // 生成该保护棋子的所有合法走法，判断是否能走到目标位置（保护）
            TArray<FChessMove2P> ProtectMoves = GenerateMovesForChess(i, j, ProtectChess);
            for (const FChessMove2P& Move : ProtectMoves)
            {
                if (Move.to.X == X && Move.to.Y == Y)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool UAI2P::IsPieceRooted(TWeakObjectPtr<AChesses> TargetChess, EChessColor Color)
{
    if (!TargetChess.IsValid() || TargetChess->GetColor() != Color) return false;

    Position TargetChessPos = TargetChess->GetSimpPosition(); // 目标棋子的位置

    // 遍历己方所有棋子，判断是否能保护该位置
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> ProtectChess = GetChess(i, j);
            if (!ProtectChess.IsValid() || ProtectChess->GetColor() != Color) continue;

            // 生成该保护棋子的所有合法走法，判断是否能走到目标位置（保护）
            TArray<FChessMove2P> ProtectMoves = GenerateMovesForChess(i, j, ProtectChess);
            for (const FChessMove2P& Move : ProtectMoves)
            {
                if (Move.to == TargetChessPos)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// 评估吃子后的安全性（是否会被反吃）
bool UAI2P::IsCaptureSafe(const FChessMove2P& Move, EChessColor AttackerColor)
{
    EChessColor DefenderColor = (AttackerColor == EChessColor::BLACKCHESS) ?
        EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 1. 先判断是否吃将
    TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> CapturedChess = GetChess(Move.to.X, Move.to.Y);
    if (CapturedChess.IsValid())
    {
        // 如果要吃的是将，需要特别小心
        if (CapturedChess->GetType() == EChessType::JIANG)
        {
            if (IsPieceRooted(Move.to.X, Move.to.Y, AttackerColor))
            {
                // 能够实现将军，且将无法反吃
                return true;
            }
        }
    }

    // 2. 模拟吃子
    MakeTestMove(Move, MovedChess);

    // 3. 检查对方是否能反吃该位置的棋子
    bool bIsSafe = true;
    TArray<FChessMove2P> DefenderMoves = GenerateAllMoves(DefenderColor);
    for (const FChessMove2P& DefMove : DefenderMoves)
    {
        if (DefMove.to.X == Move.to.X && DefMove.to.Y == Move.to.Y)
        {
            // 会被反吃
            bIsSafe = false;
            break;
        }
    }

    // 4. 撤销模拟走法
    UndoTestMove(Move, MovedChess, CapturedChess);

    // 5. 若有根（己方保护），则即使能反吃也视为安全
    if (!bIsSafe && IsPieceRooted(Move.to.X, Move.to.Y, AttackerColor))
    {
        bIsSafe = true;
    }

    return bIsSafe;
}

// 判断棋子是否孤军深入（无支援）
bool UAI2P::IsPieceIsolated(int32 X, int32 Y, EChessColor Color)
{
    // 定义“深入对方阵地”：红方在黑方底线（X<3），黑方在红方底线（X>6）
    bool bIsDeep = (Color == EChessColor::REDCHESS && X < 3) || (Color == EChessColor::BLACKCHESS && X > 6);
    if (!bIsDeep) return false;

    // 检查周围1格内是否有己方棋子（支援）
    TArray<FIntPoint> Neighbors = {
        FIntPoint(X - 1, Y), FIntPoint(X + 1, Y),
        FIntPoint(X, Y - 1), FIntPoint(X, Y + 1)
    };
    for (const FIntPoint& Neighbor : Neighbors)
    {
        if (Neighbor.X < 0 || Neighbor.X >= 10 || Neighbor.Y < 0 || Neighbor.Y >= 9) continue;
        TWeakObjectPtr<AChesses> NeighborChess = GetChess(Neighbor.X, Neighbor.Y);
        if (NeighborChess.IsValid() && NeighborChess->GetColor() == Color)
        {
            return false; // 有支援，非孤军
        }
    }
    return true;
}

// 检查核心棋子（车、马、炮）是否被攻击
bool UAI2P::IsKeyPieceUnderAttack(EChessColor Color)
{
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OpponentColor);

    // 遍历己方核心棋子，判断是否被攻击
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != Color) continue;

            // 核心棋子：车、马、炮
            EChessType Type = Chess->GetType();
            if (Type != EChessType::JV && Type != EChessType::MA && Type != EChessType::PAO) continue;

            // 检查对方是否能走到该位置（攻击）
            for (const FChessMove2P& OppMove : OpponentMoves)
            {
                if (OppMove.to.X == i && OppMove.to.Y == j)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// 检查是否是马的好位置
bool UAI2P::IsGoodHorsePosition(int32 X, int32 Y, EChessColor Color)
{
    // 马的好位置：河口、卧槽、钓鱼马等
    TArray<FIntPoint> goodHorsePositions = {
        FIntPoint(2, 2), FIntPoint(2, 6),  // 红方河口马
        FIntPoint(7, 2), FIntPoint(7, 6),  // 黑方河口马
        FIntPoint(3, 3), FIntPoint(3, 5),  // 相肩马
        FIntPoint(6, 3), FIntPoint(6, 5)   // 黑方相肩马
    };

    for (const FIntPoint& pos : goodHorsePositions)
    {
        if (X == pos.X && Y == pos.Y) return true;
    }

    return false;
}

// 检查是否是炮的威胁位置
bool UAI2P::IsThreateningPaoPosition(int32 X, int32 Y, EChessColor Color)
{
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 检查炮在这个位置是否能威胁对方棋子
    TArray<FChessMove2P> threatMoves;
    TWeakObjectPtr<AChesses> cannon = GetChess(X, Y);
    if (cannon.IsValid() && cannon->GetType() == EChessType::PAO)
    {
        threatMoves = GenerateMovesForChess(X, Y, cannon);
    }

    for (const FChessMove2P& move : threatMoves)
    {
        TWeakObjectPtr<AChesses> target = GetChess(move.to.X, move.to.Y);
        if (target.IsValid() && target->GetColor() == OpponentColor)
        {
            // 威胁高价值目标
            if (target->GetType() == EChessType::JV ||
                target->GetType() == EChessType::MA ||
                target->GetType() == EChessType::PAO ||
                target->GetType() == EChessType::JIANG)
            {
                return true;
            }
        }
    }

    // 检查是否是战略位置（中路、对方半场等）
    if (Y == 4) return true; // 中路

    // 对方半场
    if ((Color == EChessColor::REDCHESS && X <= 4) ||
        (Color == EChessColor::BLACKCHESS && X >= 5))
    {
        return true;
    }

    return false;
}

bool UAI2P::IsRiskyFaceToFaceCheck(const FChessMove2P& Move, EChessColor Color)
{
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ?
        EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 检查移动的棋子类型（只有车、马、炮、兵可能将军）
    TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
    if (!MovedChess.IsValid()) return false;

    // 模拟走法
    TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
    MakeTestMove(Move, Moved);

    // 检查是否将军
    bool bIsCheck = IsInCheck(OpponentColor);

    if (bIsCheck)
    {
        // 获取对方将的位置
        int32 OppKingX, OppKingY;
        if (GetKingPosition(OpponentColor, OppKingX, OppKingY))
        {
            // 计算移动后的棋子与对方将的距离
            int32 Distance = FMath::Abs(Move.to.X - OppKingX) + FMath::Abs(Move.to.Y - OppKingY);

            // 如果是贴脸将军（距离为1），且移动的棋子没有保护
            if (Distance == 1)
            {
                // 检查对方将是否能吃掉这个棋子
                TWeakObjectPtr<AChesses> OppKing = GetChess(OppKingX, OppKingY);
                if (OppKing.IsValid())
                {
                    TArray<FChessMove2P> KingMoves = GenerateMovesForChess(OppKingX, OppKingY, OppKing);
                    for (const FChessMove2P& KingMove : KingMoves)
                    {
                        if (KingMove.to.X == Move.to.X && KingMove.to.Y == Move.to.Y)
                        {
                            // 将可以吃掉这个棋子，检查棋子是否有保护
                            bool bHasProtection = IsPieceRooted(Move.to.X, Move.to.Y, Color);
                            UndoTestMove(Move, Moved, Captured);
                            return !bHasProtection; // 无保护就是高风险
                        }
                    }
                }
            }
        }
    }

    UndoTestMove(Move, Moved, Captured);
    return false;
}

// 找出攻击该位置的敌方棋子
TWeakObjectPtr<AChesses> UAI2P::FindAttacker(FIntPoint Position, EChessColor AttackerColor) 
{
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(AttackerColor);
    for (const FChessMove2P& Move : OpponentMoves) 
    {
        if (Move.to.X == Position.X && Move.to.Y == Position.Y) 
        {
            return GetChess(Move.from.X, Move.from.Y);
        }
    }
    return nullptr;
}

// 缓存将/帅位置（优化：避免重复遍历棋盘）
bool UAI2P::GetKingPosition(EChessColor Color, int32& OutX, int32& OutY)
{
    OutX = -1;
    OutY = -1;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (Chess.IsValid() && Chess->GetType() == EChessType::JIANG && Chess->GetColor() == Color)
            {
                OutX = i;
                OutY = j;
                return true;
            }
        }
    }
    return false;
}

void UAI2P::UpdateTranspositionTable(int32 Depth, int32 BestValue, const FChessMove2P& BestMove, int32 Alpha, int32 Beta)
{
    uint64 ZobristKey = GenerateZobristKey();
    FTranspositionEntry NewEntry = FTranspositionEntry();
    NewEntry.ZobristKey = ZobristKey;
    NewEntry.Depth = Depth;
    NewEntry.Value = BestValue;
    NewEntry.BestMove = BestMove;

    if (BestValue <= Alpha) NewEntry.Flag = ETranspositionFlag::Alpha;
    else if (BestValue >= Beta) NewEntry.Flag = ETranspositionFlag::Beta;
    else NewEntry.Flag = ETranspositionFlag::Exact;

    TranspositionTable.Add(ZobristKey, NewEntry);
}

// 校验：走指定棋步后，己方是否会被将军
bool UAI2P::IsInCheckAfterMove(const FChessMove2P& Move, EChessColor SelfColor)
{
    // 1. 保存被吃棋子（用于回滚）
    TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
    TWeakObjectPtr<AChesses> CapturedChess = GetChess(Move.to.X, Move.to.Y);
    // 2. 模拟走棋
    MakeTestMove(Move, MovedChess);
    // 3. 检查走棋后己方是否被将军
    bool bIsSelfCheck = IsInCheck(SelfColor);
    // 4. 回滚棋盘
    UndoTestMove(Move, MovedChess, CapturedChess);

    return bIsSelfCheck;
}

// 校验：当前局面下，我方下一步是否能直接吃掉对方将
bool UAI2P::CanCaptureGeneralInNextStep(EChessColor SelfColor)
{
    EChessColor OppColor = (SelfColor == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;
    // 1. 找到对方将的位置
    FVector2D GeneralPos = FVector2D(-1, -1);
    for (int32 X = 0; X < 10; X++)
    {
        for (int32 Y = 0; Y < 9; Y++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(X, Y);
            if (Chess.IsValid() && Chess->GetColor() == OppColor && Chess->GetType() == EChessType::JIANG)
            {
                GeneralPos = FVector2D(X, Y);
                break;
            }
        }
        if (GeneralPos.X != -1) break;
    }
    if (GeneralPos.X == -1) return false; // 未找到将（理论上不会发生）

    // 2. 生成我方所有合法走法，判断是否有走法能直接走到将的位置
    TArray<FChessMove2P> SelfMoves = GenerateAllMoves(SelfColor);
    for (const FChessMove2P& Move : SelfMoves)
    {
        // 走法终点是将的位置 → 能吃将
        if (Move.to.X == GeneralPos.X && Move.to.Y == GeneralPos.Y)
        {
            // 额外校验：走该步不会自将（避免吃将后自己被将）
            if (!IsInCheckAfterMove(Move, SelfColor))
            {
                return true;
            }
        }
    }

    return false;
}

// 检查是否将军
bool UAI2P::IsInCheck(EChessColor Color)
{
    int32 KingX, KingY;
    if (!GetKingPosition(Color, KingX, KingY)) return false;

    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ?
        EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& Move : OpponentMoves)
    {
        if (Move.to.X == KingX && Move.to.Y == KingY)
        {
            return true;
        }
    }
    return false;
}

// 获取游戏阶段
EChessGamePhase UAI2P::GetGamePhase()
{
    // 统计剩余棋子数量（简化逻辑：可根据实际需求调整）
    int32 TotalPieces = 0;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            if (GetChess(i, j).IsValid()) TotalPieces++;
        }
    }

    if (TotalPieces >= 28) return EChessGamePhase::Opening;   // 开局（棋子多）
    else if (TotalPieces >= 10) return EChessGamePhase::Midgame; // 中局
    else return EChessGamePhase::Endgame;                     // 残局（棋子少）
}

// 获取棋子基础价值（随游戏阶段调整）
int32 UAI2P::GetPieceBaseValue(EChessType PieceType, EChessGamePhase Phase, EChessColor AiColor)
{
    switch (PieceType)
    {
    case EChessType::JIANG:
        return VALUE_JIANG; // 将帅价值始终最高，不调整
    case EChessType::JV:
    {
        // 双车形态时，车价值加权（鼓励保留/使用双车）
        int32 JVCount = 0;
        // 复用现有逻辑统计己方车数量（无新函数/变量）
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::JV)
                {
                    JVCount++;
                }
            }
        }
        // 双车时价值+200，单车载残局+50
        if (JVCount >= 2) return VALUE_CHE + 200;
        return Phase == EChessGamePhase::Endgame ? VALUE_CHE + 50 : VALUE_CHE;
    }
    case EChessType::MA:
        // 残局马价值微调（马在残局灵活性提升）
        return Phase == EChessGamePhase::Endgame ? VALUE_MA + 30 : VALUE_MA;
    case EChessType::PAO:
    {
        // 双炮形态时，炮价值加权（鼓励保留/使用双炮）
        int32 PAOCount = 0;
        // 复用现有逻辑统计己方炮数量
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::PAO)
                {
                    PAOCount++;
                }
            }
        }
        // 双炮时价值+200，单炮在残局-50
        if (PAOCount >= 2) return VALUE_PAO + 200;
        return Phase == EChessGamePhase::Endgame ? VALUE_PAO - 50 : VALUE_PAO;
    }
    case EChessType::SHI:
        // 残局士价值降低（士仅护帅，残局帅暴露后作用下降）
        return Phase == EChessGamePhase::Endgame ? VALUE_SHI - 50 : VALUE_SHI;
    case EChessType::XIANG:
        // 残局象价值大幅降低（象无法过河，残局作用极小）
        return Phase == EChessGamePhase::Endgame ? VALUE_XIANG - 80 : VALUE_XIANG;
    case EChessType::BING:
    {
        // 细化兵的价值梯度：未过河 < 过河 < 进入九宫
        bool bIsCrossed = false; // 假设调用方可传递兵的位置，此处简化逻辑（实际可结合坐标判断）
        bool bInPalace = false;  // 兵进入对方九宫（九宫范围：X/Y需结合红黑方判断）
        // 示例：红兵九宫（X=0-2, Y=3-5），黑兵九宫（X=7-9, Y=3-5）
        // 此处复用棋盘坐标逻辑，仅做价值分层
        if (Phase == EChessGamePhase::Endgame)
        {
            if (bInPalace) return VALUE_BING_CROSSED + 200; // 九宫兵价值最高
            if (bIsCrossed) return VALUE_BING_CROSSED + 100; // 过河兵次之
            return VALUE_BING + 50; // 未过河兵小幅提升
        }
        // 中局价值
        if (bInPalace) return VALUE_BING_CROSSED + 100;
        if (bIsCrossed) return VALUE_BING_CROSSED;
        return VALUE_BING;
    }
    default: return 0;
    }
}

// 获取棋子位置价值（新增关键点位/协同性）
int32 UAI2P::GetPiecePositionValue(EChessType PieceType, EChessColor Color, int32 X, int32 Y)
{
    int32 PosValue = 0;

    // 原有逻辑：位置+有根+孤军
    if (PieceType == EChessType::JV)
    {
        if (Y == 4) PosValue += 50; // 中路车价值高
        if (X >= 2 && X <= 7) PosValue += 30; // 活跃位置
    }
    else if (PieceType == EChessType::MA)
    {
        PosValue += 20;
        // 马在好位置（如河口、卧槽位）额外加分
        if ((X == 2 && (Y == 2 || Y == 6)) || (X == 7 && (Y == 2 || Y == 6)))
            PosValue += 50;
    }
    else if (PieceType == EChessType::PAO)
    {
        // === 重点修改：炮的位置价值更加谨慎 ===
        if (Y == 4) // 中路位置
        {
            // 只有当中路有实际威胁时才加分
            bool bHasThreat = false;

            // 检查横向是否有攻击机会
            for (int32 targetY = 0; targetY < 9; targetY++)
            {
                if (targetY == Y) continue;

                TWeakObjectPtr<AChesses> Target = GetChess(X, targetY);
                if (Target.IsValid() && Target->GetColor() != Color)
                {
                    // 检查中间是否有炮架
                    int32 PawnCount = 0;
                    int32 startY = FMath::Min(targetY, Y) + 1;
                    int32 endY = FMath::Max(targetY, Y) - 1;
                    for (int32 yy = startY; yy <= endY; yy++)
                    {
                        if (GetChess(X, yy).IsValid()) PawnCount++;
                    }
                    if (PawnCount == 1) // 有炮架，能攻击
                    {
                        bHasThreat = true;
                        break;
                    }
                }
            }

            // 检查纵向是否有攻击机会
            if (!bHasThreat)
            {
                for (int32 targetX = 0; targetX < 10; targetX++)
                {
                    if (targetX == X) continue;

                    TWeakObjectPtr<AChesses> Target = GetChess(targetX, Y);
                    if (Target.IsValid() && Target->GetColor() != Color)
                    {
                        int32 PawnCount = 0;
                        int32 startX = FMath::Min(targetX, X) + 1;
                        int32 endX = FMath::Max(targetX, X) - 1;
                        for (int32 xx = startX; xx <= endX; xx++)
                        {
                            if (GetChess(xx, Y).IsValid()) PawnCount++;
                        }
                        if (PawnCount == 1)
                        {
                            bHasThreat = true;
                            break;
                        }
                    }
                }
            }

            if (bHasThreat)
                PosValue += 30; // 有实际威胁的中路炮加分
            else
                PosValue -= 20; // 无威胁的中路位置反而减分
        }
        else
        {
            // 非中路位置：只有有威胁时才加分
            bool bHasThreat = false;

            // 检查该位置炮是否能立即攻击
            TArray<FChessMove2P> ThreatMoves;
            if (GetChess(X, Y).IsValid())
            {
                ThreatMoves = GenerateMovesForChess(X, Y, GetChess(X, Y));
            }

            for (const FChessMove2P& Move : ThreatMoves)
            {
                TWeakObjectPtr<AChesses> Target = GetChess(Move.to.X, Move.to.Y);
                if (Target.IsValid() && Target->GetColor() != Color)
                {
                    bHasThreat = true;
                    break;
                }
            }

            if (bHasThreat)
                PosValue += 15; // 边路有威胁的位置小幅加分
            else
                PosValue -= 10; // 无威胁的边路位置减分
        }

        // 炮在己方阵地价值较低（防守作用有限）
        bool bInHomeTerritory = (Color == EChessColor::REDCHESS) ? (X > 4) : (X < 5);
        if (bInHomeTerritory)
            PosValue -= 15;
    }
    else if (PieceType == EChessType::BING)
    {
        bool bCrossed = (Color == EChessColor::REDCHESS) ? (X < 5) : (X >= 5);
        if (bCrossed) PosValue += VALUE_BING_CROSSED - VALUE_BING;

        // 兵在对方九宫价值大幅提升
        bool bInOpponentPalace = false;
        if (Color == EChessColor::REDCHESS)
            bInOpponentPalace = (X >= 7 && X <= 9) && (Y >= 3 && Y <= 5);
        else
            bInOpponentPalace = (X >= 0 && X <= 2) && (Y >= 3 && Y <= 5);

        if (bInOpponentPalace) PosValue += 100;
    }
    else if (PieceType == EChessType::XIANG)
    {
        if (GamePhase != EChessGamePhase::Opening)
        {
            // 象在边路位置价值大幅降低（避免无意义飞边）
            if (Y == 0 || Y == 8) {
                PosValue -= 80; // 边路象容易受制，大幅扣分
            }

            // 象在中路和象眼位置价值提高
            if (Y == 2 || Y == 6) {
                PosValue += 40; // 象眼位置是好位置
            }

            // 象在己方阵地价值更高（防守作用）
            bool bInHomeTerritory = (Color == EChessColor::REDCHESS) ? (X >= 5) : (X <= 4);
            if (bInHomeTerritory) {
                PosValue += 50;

                // 特别奖励保护中兵的象位
                if ((Color == EChessColor::REDCHESS && X == 7 && (Y == 2 || Y == 6)) ||
                    (Color == EChessColor::BLACKCHESS && X == 2 && (Y == 2 || Y == 6))) {
                    PosValue += 30;
                }
            }
        }
}

    // 通用评估：孤军和有根
    if (IsPieceIsolated(X, Y, Color))
        PosValue += PENALTY_ISOLATED_PIECE;
    if (IsPieceRooted(X, Y, Color))
        PosValue += BONUS_ROOTED_PIECE;

    // 控制关键点位加分（但炮的控制需要更严格的条件）
    if (PieceType != EChessType::PAO) // 炮的控制评估已经在上面单独处理
    {
        if (IsControlKeyPoint(X, Y, EKeyChessPoint::Center, Color))
            PosValue += 100;
        if (IsControlKeyPoint(X, Y, EKeyChessPoint::KingGate, Color))
            PosValue += 150;
        if (IsControlKeyPoint(X, Y, EKeyChessPoint::SoldierLine, Color))
            PosValue += 80;
        if (IsControlKeyPoint(X, Y, EKeyChessPoint::HorsePoint, Color))
            PosValue += 70;
    }
    else
    {
        // 炮控制关键点位的特殊评估：必须有实际威胁
        if (IsControlKeyPoint(X, Y, EKeyChessPoint::Center, Color) ||
            IsControlKeyPoint(X, Y, EKeyChessPoint::KingGate, Color))
        {
            // 检查炮是否能立即攻击到重要目标
            bool bCanAttackKeyTarget = false;
            TArray<FChessMove2P> AttackMoves;
            if (GetChess(X, Y).IsValid())
            {
                AttackMoves = GenerateMovesForChess(X, Y, GetChess(X, Y));
            }

            for (const FChessMove2P& Move : AttackMoves)
            {
                TWeakObjectPtr<AChesses> Target = GetChess(Move.to.X, Move.to.Y);
                if (Target.IsValid() && Target->GetColor() != Color)
                {
                    // 攻击到对方车、马、炮、将等高价值目标
                    if (Target->GetType() == EChessType::JV ||
                        Target->GetType() == EChessType::MA ||
                        Target->GetType() == EChessType::PAO ||
                        Target->GetType() == EChessType::JIANG)
                    {
                        bCanAttackKeyTarget = true;
                        break;
                    }
                }
            }

            if (bCanAttackKeyTarget)
                PosValue += 60; // 有实际威胁的关键点位加分
            else
                PosValue -= 30; // 无威胁的关键点位反而减分
        }
    }

    return PosValue;
}

// 获取棋子位置价值
int32 UAI2P::EvaluateBoard(EChessColor AiColor)
{
    // === 优先级1：致命威胁检测 ===
    if (IsImmediateDoublePaoMate(AiColor)) return -10000;
    if (CanCaptureGeneralInNextStep(AiColor)) return BONUS_CHECK;
    if (IsInCheck(AiColor)) return PENALTY_IN_CHECK;

    // === 优先级2：简化评估流程 ===
    int32 TotalScore = 0;

    // 1. 基础棋子价值（简化计算）
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid()) continue;

            int32 PieceValue = GetPieceBaseValue(Chess->GetType(), GamePhase, AiColor);
            TotalScore += (Chess->GetColor() == AiColor) ? PieceValue : -PieceValue;
        }
    }

    // 2. 阶段专项评估
    if (GamePhase == EChessGamePhase::Opening) {
        TotalScore += EvaluateOpeningFeatures(AiColor);
    }
    else {
        TotalScore += EvaluateMidEndgameFeatures(AiColor);
    }

    // 3. 将军/被将军状态
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    if (IsInCheck(OpponentColor)) TotalScore += BONUS_CHECK;

    return TotalScore;
}

// 被将军时的走法排序
TArray<FChessMove2P> UAI2P::SortMovesWhenInCheck(const TArray<FChessMove2P>& Moves, EChessColor Color)
{
    if (Moves.Num() <= 1)
        return Moves;

    EChessColor OppColor = (Color == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;
    TArray<FMoveScore> MoveScores;

    // 1. 获取关键信息
    int32 KingX, KingY;
    if (!GetKingPosition(Color, KingX, KingY))
        return Moves;

    // 找到攻击将的棋子
    TWeakObjectPtr<AChesses> Attacker = nullptr;
    TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OppColor);
    for (const FChessMove2P& OppMove : OpponentMoves)
    {
        if (OppMove.to.X == KingX && OppMove.to.Y == KingY)
        {
            Attacker = GetChess(OppMove.from.X, OppMove.from.Y);
            break;
        }
    }

    // 2. 对每个走法进行安全评估
    for (const FChessMove2P& Move : Moves)
    {
        int32 Score = 0;
        TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);

        if (!MovedChess.IsValid())
        {
            MoveScores.Add(FMoveScore(Move, -10000));
            continue;
        }

        // === 核心安全检测：避免送子 ===
        bool bIsSafeMove = true;

        // 2.1 检查是否会导致高价值棋子被低价值棋子吃掉
        if (MovedChess->GetType() != EChessType::JIANG)
        {
            TWeakObjectPtr<AChesses> Target = GetChess(Move.to.X, Move.to.Y);

            // 如果是吃子走法，确保安全
            if (Target.IsValid() && Target->GetColor() == OppColor)
            {
                int32 MovedValue = GetPieceBaseValue(MovedChess->GetType(), GamePhase, Color);
                int32 TargetValue = GetPieceBaseValue(Target->GetType(), GamePhase, OppColor);

                // 高风险检测：用高价值棋子吃低价值有保护棋子
                if (MovedValue > TargetValue * 2 && IsPieceRooted(Move.to.X, Move.to.Y, OppColor))
                {
                    Score -= 2000; // 严重送子惩罚
                    bIsSafeMove = false;
                }
            }
        }

        // 2.2 检查走法后是否仍然被将军
        bool bStillInCheck = IsInCheckAfterMove(Move, Color);

        if (bStillInCheck)
        {
            // === 未能解将的走法 ===
            Score -= 5000; // 严重惩罚

            // 即使未能解将，也要避免送子
            if (!bIsSafeMove)
            {
                Score -= 3000; // 额外送子惩罚
            }

            MoveScores.Add(FMoveScore(Move, Score));
            continue;
        }

        // === 解将成功的走法 ===
        Score += 1500; // 基础解将奖励

        // 3. 安全性优先的解将策略
        if (bIsSafeMove)
        {
            // 3.1 将帅移动是最安全的解将方式
            if (MovedChess->GetType() == EChessType::JIANG)
            {
                Score += 800; // 将帅移动额外奖励

                // 评估将帅移动后的安全位置
                int32 SafetyScore = EvaluateKingSafety(Move.to.X, Move.to.Y, Color);
                Score += SafetyScore;
            }
            // 3.2 吃掉攻击者（但必须安全）
            else if (Attacker.IsValid() &&
                Move.to.X == Attacker->GetSimpPosition().X &&
                Move.to.Y == Attacker->GetSimpPosition().Y)
            {
                // 确保吃子是安全的
                if (IsCaptureSafe(Move, Color))
                {
                    Score += 600; // 安全吃攻击者奖励

                    // 价值判断：用低价值棋子吃高价值攻击者更优
                    int32 MovedValue = GetPieceBaseValue(MovedChess->GetType(), GamePhase, Color);
                    int32 AttackerValue = GetPieceBaseValue(Attacker->GetType(), GamePhase, OppColor);

                    if (MovedValue < AttackerValue)
                    {
                        Score += (AttackerValue - MovedValue) * 2; // 划算交换奖励
                    }
                }
                else
                {
                    Score -= GetPieceBaseValue(MovedChess->GetType(), GamePhase, Color); // 不安全吃子惩罚
                }
            }
            // 3.3 垫将（阻挡）
            else
            {
                // 检查是否是有效的阻挡
                bool bEffectiveBlock = false;

                // 炮的攻击需要炮架阻挡
                if (Attacker.IsValid() && Attacker->GetType() == EChessType::PAO)
                {
                    // 检查移动后的位置是否在炮和将之间
                    bEffectiveBlock = BlocksPaoLine(Move, Color);
                }
                // 车的直线攻击需要直线阻挡
                else if (Attacker.IsValid() && Attacker->GetType() == EChessType::JV)
                {
                    if ((Move.to.X == KingX && KingX == Attacker->GetSimpPosition().X) ||
                        (Move.to.Y == KingY && KingY == Attacker->GetSimpPosition().Y))
                    {
                        bEffectiveBlock = BlocksCheLine(Move, Color, Attacker);
                    }
                }

                if (bEffectiveBlock)
                {
                    Score += 400; // 有效阻挡奖励

                    // 低价值棋子垫将更优
                    int32 BlockerValue = GetPieceBaseValue(MovedChess->GetType(), GamePhase, Color);
                    if (BlockerValue <= 200) // 兵、士、象等低价值棋子
                    {
                        Score += 200;
                    }
                }
            }

            // 3.4 解将后的局面评估
            TWeakObjectPtr<AChesses> Moved = GetChess(Move.from.X, Move.from.Y);
            TWeakObjectPtr<AChesses> Captured = GetChess(Move.to.X, Move.to.Y);
            MakeTestMove(Move, Moved);

            // 检查解将后是否能反将
            if (IsInCheck(OppColor))
            {
                Score += 300; // 解将同时反将奖励
            }

            // 避免解将后立即面临新的威胁
            if (IsKeyPieceUnderAttack(Color))
            {
                Score -= 200; // 解将后核心棋子被攻击惩罚
            }

            UndoTestMove(Move, Moved, Captured);
        }
        else
        {
            // 不安全走法大幅降权，但不禁用（可能是唯一解）
            Score -= 1000;
        }

        MoveScores.Add(FMoveScore(Move, Score));
    }

    // 4. 按评分排序，但确保安全走法优先
    MoveScores.Sort([](const FMoveScore& A, const FMoveScore& B)
        {
            // 安全走法优先：正分>负分，同为正分时高分优先
            if (A.Score >= 0 && B.Score < 0) return true;
            if (A.Score < 0 && B.Score >= 0) return false;
            return A.Score > B.Score;
        });

    // 5. 提取排序后的走法
    TArray<FChessMove2P> SortedMoves;
    for (const FMoveScore& MS : MoveScores)
    {
        // 只保留评分不是极端负分的走法（避免完全无解）
        if (MS.Score > -4000)
        {
            SortedMoves.Add(MS.Move);
        }
    }

    // 如果所有走法都被过滤，返回原始排序（避免无走法可选）
    if (SortedMoves.Num() == 0)
    {
        return Moves;
    }

    return SortedMoves;
}

// 走法排序（优先搜索高价值走法，提升α-β剪枝效率）
TArray<FChessMove2P> UAI2P::SortMoves(const TArray<FChessMove2P>& Moves, EChessColor Color)
{
    if (Moves.Num() <= 1)
        return Moves;

    EChessColor OppColor = (Color == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;

    // 按难度调整随机阈值
    int32 RANDOM_THRESHOLD = 0;
    int32 RANDOM_TWEAK_RANGE = 0;
    switch (AIDifficulty)
    {
    case EAI2PDifficulty::Easy:
        RANDOM_THRESHOLD = 200; // 宽范围随机
        RANDOM_TWEAK_RANGE = 20; // 大扰动
        break;
    case EAI2PDifficulty::Normal:
        RANDOM_THRESHOLD = 50;  // 中等范围
        RANDOM_TWEAK_RANGE = 10; // 中扰动
        break;
    case EAI2PDifficulty::Hard:
        RANDOM_THRESHOLD = 5;
        RANDOM_TWEAK_RANGE = 2;
        break;
    }

    // ===== 重新排序：将军且能吃将 > 普通走法 =====
    // 1. 先加将军且能吃将的走法
    TArray<FChessMove2P> SortedMoves;
    // 2. 普通走法按原有评分排序后追加
    TArray<FMoveScore> MoveScores;

    // 1. 找出当前被攻击的己方棋子
    TArray<FIntPoint> AttackedPieces;
    TArray<FIntPoint> Attackers;
    TArray<int32> ThreatLevels; // 威胁等级（基于棋子价值）

    for (int32 i = 0; i < 10; i++) 
    {
        for (int32 j = 0; j < 9; j++) 
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (!Chess.IsValid()) continue;

            // 检查这个棋子是否被对方攻击
            bool IsUnderAttack = false;
            FIntPoint AttackerPos;
            int32 ThreatLevel = 0;

            // 生成对方所有走法，检查是否能攻击到这个棋子
            TArray<FChessMove2P> OpponentMoves = GenerateAllMoves(OppColor);
            for (const FChessMove2P& OppMove : OpponentMoves) 
            {
                if (OppMove.to.X == i && OppMove.to.Y == j) 
                {
                    TWeakObjectPtr<AChesses> Attacker = GetChess(OppMove.from.X, OppMove.from.Y);
                    if (Attacker.IsValid()) {
                        IsUnderAttack = true;
                        AttackerPos = FIntPoint(OppMove.from.X, OppMove.from.Y);

                        // 计算威胁等级：攻击者价值 vs 被攻击棋子价值
                        int32 AttackerValue = GetPieceBaseValue(Attacker->GetType(), GamePhase, OppColor);
                        int32 DefenderValue = GetPieceBaseValue(Chess->GetType(), GamePhase, Color);
                        ThreatLevel = DefenderValue - AttackerValue; // 正数表示吃亏，负数表示划算
                        break;
                    }
                }
            }

            if (IsUnderAttack) {
                AttackedPieces.Add(FIntPoint(i, j));
                Attackers.Add(AttackerPos);
                ThreatLevels.Add(ThreatLevel);
            }
        }
    }

    for (const FChessMove2P& Move : Moves)
    {
        int32 Score = 0;

        TWeakObjectPtr<AChesses> Chess = GetChess(Move.from.X, Move.from.Y);
        TWeakObjectPtr<AChesses> TargetChess = GetChess(Move.to.X, Move.to.Y);
        int32 MyValue = 0;

        int32 ProtectionScore = 0;

        if (TargetChess.IsValid() && TargetChess->GetType() == EChessType::JIANG)
        {
            if (Color == GlobalAIColor && TargetChess->GetColor() != Color) // 执棋方验证
            {
                Score = -PENALTY_IN_CHECK; // 这是一个绝杀的步骤
                MoveScores.Add(FMoveScore(Move, Score));
                continue;
            }
        }

        if (IsInCheckAfterMove(Move, Color))
        {
            Score = -VALUE_JIANG;
            MoveScores.Add(FMoveScore(Move, Score));
            continue;
        }


        if (IsRiskyFaceToFaceCheck(Move, Color)) // 会被反吃的无意义走法
        {
            Score = -VALUE_JIANG;
            MoveScores.Add(FMoveScore(Move, Score));
            continue;
        }

        if (IsInCheckAfterMove(Move, OppColor)) // 走该步后对方被将军
        {
            // 判断我方下一步是否能吃将且不被反吃不自将
            if (!IsInCheckAfterMove(Move, Color) && IsCaptureSafe(Move, Color))
            {
                Score = 2000;
                MoveScores.Add(FMoveScore(Move, Score));
                continue;
            }
        }

        // 检查这个走法是否能保护被攻击的棋子
        for (int32 k = 0; k < AttackedPieces.Num(); k++) 
        {
            FIntPoint AttackedPos = AttackedPieces[k];
            FIntPoint AttackerPos = Attackers[k];
            int32 ThreatLevel = ThreatLevels[k];

            // 情况1：移动被攻击的棋子本身（躲避）
            if (Move.from.X == AttackedPos.X && Move.from.Y == AttackedPos.Y) 
            {
                // 躲避攻击：根据威胁等级给予奖励
                if (ThreatLevel > 0)  // 吃亏的交换，躲避很重要
                {
                    ProtectionScore += (900 + ThreatLevel * 10);
                }
                else  // 划算的交换，可以酌情考虑
                {
                    ProtectionScore += 200;
                }

                // 额外检查：移动后的位置是否安全
                MakeTestMove(Move, Chess);

                bool StillUnderAttack = false;
                TArray<FChessMove2P> NewOpponentMoves = GenerateAllMoves(OppColor);
                for (const FChessMove2P& NewOppMove : NewOpponentMoves) 
                {
                    if (NewOppMove.to.X == Move.to.X && NewOppMove.to.Y == Move.to.Y) 
                    {
                        StillUnderAttack = true;
                        break;
                    }
                }

                if (!StillUnderAttack) 
                {
                    ProtectionScore += 900; // 成功躲避到安全位置
                }

                UndoTestMove(Move, Chess, TargetChess);
            }

            // 情况2：吃掉攻击者（反打）
            else if (Move.to.X == AttackerPos.X && Move.to.Y == AttackerPos.Y) 
            {
                // 反打奖励：根据威胁等级
                if (ThreatLevel > 0) 
                {
                    ProtectionScore += 500 + ThreatLevel * 15; // 高威胁，高奖励
                }
                else 
                {
                    ProtectionScore += 450; // 低威胁，中等奖励
                }

                // 检查吃子是否安全
                if (IsCaptureSafe(Move, Color)) 
                {
                    ProtectionScore += 200;
                }
            }

            // 情况3：保护性走法（垫将、阻挡攻击线路等）
            else 
            {
                // 检查这个走法是否能阻挡攻击线路
                if (BlocksPaoLine(Move, Color)) 
                {
                    ProtectionScore += 350;
                }

                // 检查是否能保护被攻击的棋子（形成保护）
                MakeTestMove(Move, Chess);

                // 检查移动后，被攻击的棋子是否有保护了
                if (IsPieceRooted(AttackedPos.X, AttackedPos.Y, Color)) 
                {
                    ProtectionScore += 250;
                }

                UndoTestMove(Move, Chess, TargetChess);
            }
        }

        // 将保护分数加入到总评分中
        Score += ProtectionScore;

        // 在原有的评分计算后，添加被攻击棋子的特殊处理
        if (Chess.IsValid()) 
        {
            // 如果移动的棋子本身是被攻击的棋子，给予额外考量
            FIntPoint FromPos = FIntPoint(Move.from.X, Move.from.Y);
            if (AttackedPieces.Contains(FromPos)) 
            {
                // 被攻击的棋子移动优先级提升
                Score += 200;

                // 如果是高价值棋子被低价值棋子攻击，躲避优先级最高
                int32 PieceValue = GetPieceBaseValue(Chess->GetType(), GamePhase, Color);
                TWeakObjectPtr<AChesses> Attacker = FindAttacker(FromPos, OppColor);
                if (Attacker.IsValid()) 
                {
                    int32 AttackerValue = GetPieceBaseValue(Attacker->GetType(), GamePhase, OppColor);
                    if (PieceValue > AttackerValue * 2) // 高价值被低价值攻击
                    { 
                        Score += 500; // 大幅提升躲避优先级
                    }
                }
            }
        }

        // 针对炮的特殊处理
        if (Chess.IsValid() && Chess->GetType() == EChessType::PAO) {
            // 炮的移动需要特别小心对方的车

            // 检查移动后是否处于对方车的直线攻击范围内
            MakeTestMove(Move, Chess);

            // 检查横向和纵向是否有对方车威胁
            bool bUnderCheThreat = false;
            for (int32 i = 0; i < 10; i++) {
                for (int32 j = 0; j < 9; j++) {
                    TWeakObjectPtr<AChesses> threatChess = GetChess(i, j);
                    if (threatChess.IsValid() && threatChess->GetColor() == OppColor &&
                        threatChess->GetType() == EChessType::JV) {

                        // 检查车是否能直线攻击到炮的新位置
                        if (i == Move.to.X || j == Move.to.Y) {
                            TArray<FChessMove2P> cheMoves = GenerateMovesForChess(i, j, threatChess);
                            for (const FChessMove2P& cheMove : cheMoves) {
                                if (cheMove.to.X == Move.to.X && cheMove.to.Y == Move.to.Y) {
                                    bUnderCheThreat = true;
                                    break;
                                }
                            }
                        }
                        if (bUnderCheThreat) break;
                    }
                }
                if (bUnderCheThreat) break;
            }

            UndoTestMove(Move, Chess, TargetChess);
            if (bUnderCheThreat) {
                // 处于车的攻击线，检查是否有保护
                if (!IsPieceRooted(Move.to.X, Move.to.Y, Color)) {
                    // 无保护，大幅扣分（避免炮被车吃）
                    Score -= 800;
                }
                else {
                    // 有保护，但仍需谨慎
                    Score -= 100;
                }
            }

            // 特别检查：炮移动到对方马的位置的风险
            if (!TargetChess.IsValid()) {
                // 空移动，检查是否靠近对方马
                for (int32 i = 0; i < 10; i++) {
                    for (int32 j = 0; j < 9; j++) {
                        TWeakObjectPtr<AChesses> nearbyChess = GetChess(i, j);
                        if (nearbyChess.IsValid() && nearbyChess->GetColor() == OppColor &&
                            nearbyChess->GetType() == EChessType::MA) {

                            // 检查马是否能吃到炮的新位置
                            TArray<FChessMove2P> maMoves = GenerateMovesForChess(i, j, nearbyChess);
                            for (const FChessMove2P& maMove : maMoves) {
                                if (maMove.to.X == Move.to.X && maMove.to.Y == Move.to.Y) {
                                    // 马能吃到炮的位置，检查炮是否有保护
                                    if (!IsPieceRooted(Move.to.X, Move.to.Y, Color)) {
                                        Score -= 400; // 炮可能被马吃
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 中高优先级：安全吃子
        if (Chess.IsValid())
        {
            MyValue = GetPieceBaseValue(Chess->GetType(), GamePhase, Color);
            // 根据吃的棋子价值额外加分
            TWeakObjectPtr<AChesses> Target = GetChess(Move.to.X, Move.to.Y);
            if (Target.IsValid()) {
                int32 TargetValue = GetPieceBaseValue(Target->GetType(), GamePhase, Color);
                if (TargetValue < MyValue / 4)
                {
                    Score = 0;
                    MoveScores.Add(FMoveScore(Move, Score));
                    continue; // 价值不大，不考虑了
                }
                Score += TargetValue / 10; // 按价值比例加分
            }

            if (IsCaptureSafe(Move, Color))
            {
                Score += BONUS_SAFE_CAPTURE; // 走棋后该子是安全的
            }
            else
            {
                // 吃不到子还会被吃，垃圾走法，直接不考虑
                if (!Target.IsValid())
                {
                    Score = 0;
                    MoveScores.Add(FMoveScore(Move, Score));
                    continue;
                }

                Score -= MyValue; // 会被吃扣分
            }
        }

        // 中优先级：进攻协同和控点
        Score += EvaluateAttackSynergy(Move, Color);

        // 中低优先级：防守
        if (IsEffectiveDefenseMove(Move, Color)) {
            if (IsOpponentHasLethalAttack(Color)) {
                Score += BONUS_BLOCK_LETHAL_ATTACK;
            }
            else {
                Score += BONUS_FILL_DEFENSE_WEAKNESS;
            }
        }

        // 基础安全性评估
        if (IsPieceRooted(Move.to.X, Move.to.Y, Color)) {
            Score += BONUS_PROTECTED_PIECE;
        }

        if (IsPieceIsolated(Move.to.X, Move.to.Y, Color)) {
            Score += PENALTY_ISOLATED_PIECE;
        }

        // ========== 评分随机扰动 ==========
        int32 RandomTweak = FMath::RandRange(-RANDOM_TWEAK_RANGE, RANDOM_TWEAK_RANGE);
        Score += RandomTweak;

        MoveScores.Add(FMoveScore(Move, Score));
    }

    // 步骤1：按评分降序排序
    MoveScores.Sort([](const FMoveScore& A, const FMoveScore& B)
        {
            return A.Score > B.Score;
        });

    // 步骤2：分组处理——将评分差值≤RANDOM_THRESHOLD的走法归为一组，组内随机打乱
    TArray<TArray<FMoveScore>> ScoreGroups;
    if (MoveScores.Num() > 0)
    {
        ScoreGroups.Add({ MoveScores[0] }); // 初始化第一组
        for (int32 i = 1; i < MoveScores.Num(); i++)
        {
            FMoveScore& Current = MoveScores[i];
            FMoveScore& LastInGroup = ScoreGroups.Last()[0];
            // 当前走法与组内首条走法的评分差≤阈值 → 加入同一组
            if (LastInGroup.Score - Current.Score <= RANDOM_THRESHOLD)
            {
                ScoreGroups.Last().Add(Current);
            }
            else
            {
                ScoreGroups.Add({ Current }); // 新建组
            }
        }
    }

    // 步骤3：每组内随机打乱，再合并（保证组间仍按评分降序，组内随机）
    TArray<FMoveScore> RandomizedNormalMoves;
    for (auto& Group : ScoreGroups)
    {
        Group.Sort([](const FMoveScore& A, const FMoveScore& B) {
            // 随机打乱：返回随机bool值
            return FMath::RandBool();
            });
        RandomizedNormalMoves.Append(Group);
    }

    // 添加常规走法数组
    for (const FMoveScore& MS : RandomizedNormalMoves)
    {
        SortedMoves.Add(MS.Move);
    }
    return SortedMoves;
}

// Zobrist哈希生成
uint64 UAI2P::GenerateZobristKey()
{
    // 使用简单的FString作为键，避免哈希冲突问题
    FString BoardState;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(i, j);
            if (Chess.IsValid())
            {
                BoardState += FString::Printf(TEXT("(%d,%d,%d,%d)"),
                    i, j, (int32)Chess->GetColor(), (int32)Chess->GetType());
            }
            else
            {
                BoardState += FString::Printf(TEXT("(%d,%d,-1,-1)"), i, j, -1, -1);
            }
        }
    }

    // 使用简单的哈希函数
    uint64 Hash = 0;
    for (TCHAR c : BoardState)
    {
        Hash = (Hash * 31) + (uint64)c;
    }

    return Hash;
}

// α-β剪枝核心搜索
int32 UAI2P::AlphaBetaSearch(int32 Depth, int32 Alpha, int32 Beta, EChessColor CurrentColor, bool IsMaximizingPlayer)
{
    uint64 ZobristKey = GenerateZobristKey();
    EChessColor OpponentColor = (CurrentColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    int32 BestValue = IsMaximizingPlayer ? INT_MIN : INT_MAX;
    FChessMove2P BestMove;
    BestMove.bIsValid = false; // 初始为无效状态

    // 1. 置换表查找（命中则直接返回缓存值）
    if (TranspositionTable.Contains(ZobristKey))
    {
        FTranspositionEntry Entry = TranspositionTable[ZobristKey];
        if (Entry.Depth >= Depth)
        {
            if (Entry.Flag == ETranspositionFlag::Exact) return Entry.Value;
            else if (Entry.Flag == ETranspositionFlag::Alpha && Entry.Value <= Alpha) return Alpha;
            else if (Entry.Flag == ETranspositionFlag::Beta && Entry.Value >= Beta) return Beta;
        }
    }

    // 2. 深度为0 → 评估当前局面
    if (Depth <= 0)
    {
        int32 value = 0;

        // === 如果面临立即的双炮杀，返回极低分数 ===
        if (IsImmediateDoublePaoMate(CurrentColor))
        {
            value = IsMaximizingPlayer ? -VALUE_JIANG : VALUE_JIANG;
        }
        else
        {
            value = EvaluateBoard(CurrentColor);
        }

        UpdateTranspositionTable(Depth, value, BestMove, Alpha, Beta);
        return value;
    }

    // 3. 生成并排序走法（优先高价值走法，提升剪枝效率）
    TArray<FChessMove2P> Moves = GenerateAllMoves(CurrentColor);

    // === 将军状态下的特殊处理 ===
    bool bIsInCheck = IsInCheck(CurrentColor);
    if (bIsInCheck)
    {
        // 被将军时，优先考虑解将的走法
        TArray<FChessMove2P> EscapeMoves;
        for (const FChessMove2P& Move : Moves)
        {
            if (!IsInCheckAfterMove(Move, CurrentColor))
            {
                EscapeMoves.Add(Move);
            }
        }

        // 如果没有解将的走法，说明被将死
        if (EscapeMoves.Num() == 0)
        {
            int32 value = IsMaximizingPlayer ? -VALUE_JIANG : VALUE_JIANG;
            UpdateTranspositionTable(Depth, value, BestMove, Alpha, Beta);
            return value;
        }

        Moves = EscapeMoves;
        Moves = SortMovesWhenInCheck(Moves, CurrentColor);
    }
    else
    {
        Moves = SortMoves(Moves, CurrentColor);
    }

    // 4. 遍历所有走法，执行α-β剪枝
    for (const FChessMove2P& Move : Moves)
    {

        // 超时
        if (Clock.GetElapsedMilliseconds() > MaxTime) break;

        // 模拟走法
        TWeakObjectPtr<AChesses> MovedChess = GetChess(Move.from.X, Move.from.Y);
        TWeakObjectPtr<AChesses> CapturedChess = GetChess(Move.to.X, Move.to.Y);
        MakeTestMove(Move, MovedChess);

        // 递归搜索下一层
        int32 CurrentValue = AlphaBetaSearch(Depth - 1, Alpha, Beta, OpponentColor, !IsMaximizingPlayer);

        // 撤销走法
        UndoTestMove(Move, MovedChess, CapturedChess);

        // 更新最优值
        if (IsMaximizingPlayer)
        {
            if (CurrentValue > BestValue)
            {
                BestValue = CurrentValue;
                BestMove = Move;
            }
            Alpha = FMath::Max(Alpha, CurrentValue);
        }
        else
        {
            if (CurrentValue < BestValue)
            {
                BestValue = CurrentValue;
                BestMove = Move;
            }
            Beta = FMath::Min(Beta, CurrentValue);
        }

        // α-β剪枝：当前层已无更优解，提前退出
        if (Beta <= Alpha) break;
    }

    // 5. 更新置换表
    UpdateTranspositionTable(Depth, BestValue, BestMove, Alpha, Beta);

    return BestValue;
}

// 迭代加深搜索
int32 UAI2P::IterativeDeepeningSearch(int32 MaxDepth, EChessColor AiColor)
{
    ClearTranspositionTable(); // 每次迭代前清空置换表

    int32 BestScore = 0;
    FChessMove2P BestMove;

    Clock.Start();

    // 逐步提升搜索深度，兼顾速度和精度
    for (int32 Depth = 1; Depth <= MaxDepth; Depth++)
    {
        int32 CurrentScore = AlphaBetaSearch(Depth, INT_MIN, INT_MAX, AiColor, true);

        // 缓存当前深度的最优走法（从置换表中获取）
        uint64 CurrentKey = GenerateZobristKey();
        if (TranspositionTable.Contains(CurrentKey))
        {
            BestMove = TranspositionTable[CurrentKey].BestMove;
            BestScore = CurrentScore;
        }

        // 添加超时判断，避免深度过大导致卡顿
        if (Clock.GetElapsedMilliseconds() > MaxTime) 
        {
            ULogger::Log(FString::Printf(TEXT("已搜索至第%d层, 总用时:%fms(撞时间墙提前退出)"), Depth, Clock.GetElapsedMilliseconds()));
            break; 
        }
        ULogger::Log(FString::Printf(TEXT("已搜索至第%d层, 已用时:%fms"), Depth, Clock.GetElapsedMilliseconds()));
    }

    return BestScore;
}

// 获取AI最优走法（对外接口）
FChessMove2P UAI2P::GetBestMove(TWeakObjectPtr<UChessBoard2P> InBoard2P, EChessColor InAiColor, EAI2PDifficulty InDifficulty, int32 InMaxTime, bool bEnableMachineLearning, UChessMLModule* MLModule)
{
    SetBoard(InBoard2P);

    int32 Depth = GetSearchDepth(AIDifficulty);

    FChessMove2P MLMove;
    MLMove.bIsValid = false;
    // 如果启用机器学习，尝试使用机器学习预测
    if (bEnableMachineLearning && MLModule)
    {
        if (MLModule->LoadModel(TEXT("ChineseChess")))
        {
            FString BoardFen = GetCurrentBoardFEN();
            TArray<FString> ValidMoves = GetValidMovesAsStrings(InAiColor);

            FString MLMove_s = MLModule->PredictBestMove(BoardFen, ValidMoves);
            if (!MLMove_s.IsEmpty())
            {
                // 将字符串走法转换为FChessMove2P
                FChessMove2P Move = StringToMove(MLMove_s);
                if (Move.IsValid())
                {
                    MLMove = StringToMove(MLMove_s);
                    ULogger::Log(FString::Printf(TEXT("Using ML prediction: %s"), *MLMove_s));
                }
            }
        }
    }

    GlobalAIColor = InAiColor;
    AIDifficulty = InDifficulty;
    MaxTime = InMaxTime;
    GamePhase = GetGamePhase();

    // 执行迭代加深搜索
    IterativeDeepeningSearch(Depth, InAiColor);

    // 获取置换表Key
    uint64 OriginalKey = GenerateZobristKey();

    // 从置换表中获取最优走法
    if (TranspositionTable.Contains(OriginalKey))
    {
        FChessMove2P AIMove = TranspositionTable[OriginalKey].BestMove;
        FChessMove2P BestMove;
        if (!MLMove.IsValid())
        {
            ULogger::Log("GetBestMove", "Move use AIMove!");
            BestMove = AIMove;
        }
        else if (AIMove == MLMove)
        {
            ULogger::Log(TEXT("GetBestMove"), TEXT("Move use MLMove!"));
            BestMove = MLMove;
        }
        else
        {
            // 评估移动后的得分，看看哪个更好
            TWeakObjectPtr<AChesses> Captured;
            Captured = GetChess(AIMove.to.X, AIMove.to.Y);
            MakeTestMove(AIMove);
            int32 AIScore = EvaluateBoard(GlobalAIColor);
            UndoTestMove(AIMove, Captured);

            Captured = GetChess(MLMove.to.X, MLMove.to.Y);
            MakeTestMove(MLMove);
            int32 MLScore = EvaluateBoard(GlobalAIColor);
            UndoTestMove(MLMove, Captured);

            // 选择得分更高的作为最优数据
            if (MLScore >= AIScore)
            {
                ULogger::Log(TEXT("GetBestMove"), TEXT("Move use MLMove!"));
                BestMove = MLMove;
            }
            else
            {
                ULogger::Log("GetBestMove", "Move use AIMove!");
                BestMove = AIMove;
            }

        }

        if (BestMove.IsValid())
        {
            // 保存传统AI的决策作为训练数据
            if (bEnableMachineLearning && MLModule)
            {
                FString BoardFen = GetCurrentBoardFEN();
                FString MoveString = MoveToString(BestMove);
                float Score = EvaluateBoard(InAiColor);
                MLModule->SaveTrainingData(BoardFen, MoveString, Score, Depth);
            }
            return BestMove;
        }

        ULogger::LogWarning(FString::Printf(TEXT("TranspositionTable move: from %d,%d; to %d,%d"), BestMove.from.X, BestMove.from.Y, BestMove.to.X, BestMove.to.Y));
    }

    // 兜底：返回第一个合法走法
    TArray<FChessMove2P> Moves = GenerateAllMoves(InAiColor);
    ULogger::LogWarning(FString::Printf(TEXT("GetBestMove:Not use TranspositionTable, Moves num: %d"), Moves.Num()));
    return Moves.Num() > 0 ? Moves[0] : FChessMove2P();
}

int32 UAI2P::Evaluate(EChessColor color)
{
    return EvaluateBoard(color);
}

void UAI2P::StopThinkingImmediately()
{
    MaxTime = 0;
}

// 辅助函数：将走法转换为字符串
FString UAI2P::MoveToString(const FChessMove2P& Move) const
{
    // 将坐标转换为字符串，如 "a0a1"
    FString FromX = FString::Printf(TEXT("%c"), 'a' + Move.from.Y);
    FString FromY = FString::Printf(TEXT("%d"), Move.from.X);
    FString ToX = FString::Printf(TEXT("%c"), 'a' + Move.to.Y);
    FString ToY = FString::Printf(TEXT("%d"), Move.to.X);

    return FromX + FromY + ToX + ToY;
}

// 辅助函数：将字符串转换为走法
FChessMove2P UAI2P::StringToMove(FString MoveString) const
{
    FChessMove2P Move;
    Move.bIsValid = false;

    if (MoveString.Len() < 4) return Move;

    Move.from.Y = MoveString[0] - 'a';
    Move.from.X = FCString::Atoi(*MoveString.Mid(1, 1));
    Move.to.Y = MoveString[2] - 'a';
    Move.to.X = FCString::Atoi(*MoveString.Mid(3, 1));

    // 验证走法是否有效
    if (Move.from.X >= 0 && Move.from.X < 10 && Move.from.Y >= 0 && Move.from.Y < 9 &&
        Move.to.X >= 0 && Move.to.X < 10 && Move.to.Y >= 0 && Move.to.Y < 9)
    {
        Move.bIsValid = true;
    }

    return Move;
}

TArray<FString> UAI2P::GetValidMovesAsStrings(EChessColor Color)
{
    TArray<FString> MoveStrings;

    // 生成所有合法走法
    TArray<FChessMove2P> ValidMoves = GenerateAllMoves(Color);

    for (const FChessMove2P& Move : ValidMoves)
    {
        FString MoveStr = MoveToString(Move);
        if (!MoveStr.IsEmpty())
        {
            MoveStrings.Add(MoveStr);
        }
    }

    return MoveStrings;
}

FString UAI2P::GetCurrentBoardFEN() const
{
    FString FEN;

    // 遍历棋盘每一行（从第0行到第9行）
    for (int32 Row = 0; Row < 10; Row++)
    {
        int32 EmptyCount = 0;
        FString RowString;

        // 遍历每一列（从第0列到第8列）
        for (int32 Col = 0; Col < 9; Col++)
        {
            TWeakObjectPtr<AChesses> Chess = GetChess(Row, Col);

            if (!Chess.IsValid())
            {
                EmptyCount++;
            }
            else
            {
                // 如果有连续的空位，先添加数字
                if (EmptyCount > 0)
                {
                    RowString += FString::FromInt(EmptyCount);
                    EmptyCount = 0;
                }

                // 添加棋子字符
                RowString += GetPieceFENChar(Chess->GetType(), Chess->GetColor());
            }
        }

        // 行结束，处理剩余的空位
        if (EmptyCount > 0)
        {
            RowString += FString::FromInt(EmptyCount);
        }

        // 添加到FEN字符串
        FEN += RowString;

        // 添加行分隔符（除了最后一行）
        if (Row < 9)
        {
            FEN += TEXT("/");
        }
    }

    return FEN;
}

// 辅助函数：根据棋子类型和颜色返回FEN字符
FString UAI2P::GetPieceFENChar(EChessType PieceType, EChessColor Color) const
{
    FString PieceChar;

    switch (PieceType)
    {
    case EChessType::JIANG:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("K") : TEXT("k");
        break;
    case EChessType::SHI:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("A") : TEXT("a");
        break;
    case EChessType::XIANG:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("B") : TEXT("b");
        break;
    case EChessType::MA:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("N") : TEXT("n");
        break;
    case EChessType::JV:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("R") : TEXT("r");
        break;
    case EChessType::PAO:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("C") : TEXT("c");
        break;
    case EChessType::BING:
        PieceChar = (Color == EChessColor::REDCHESS) ? TEXT("P") : TEXT("p");
        break;
    default:
        PieceChar = TEXT("?");
        break;
    }

    return PieceChar;
}

void UAI2P::MakeTestMove(const FChessMove2P& move)
{
    SetChess(move.to.X, move.to.Y, GetChess(move.from.X, move.from.Y));
    SetChess(move.from.X, move.from.Y, nullptr);
}

void UAI2P::MakeTestMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> movedPiece)
{
    SetChess(move.to.X, move.to.Y, movedPiece);
    SetChess(move.from.X, move.from.Y, nullptr);
}

void UAI2P::UndoTestMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> capturedPiece)
{
    SetChess(move.from.X, move.from.Y, GetChess(move.to.X, move.to.Y));
    SetChess(move.to.X, move.to.Y, capturedPiece);
}

void UAI2P::UndoTestMove(const FChessMove2P& move, TWeakObjectPtr<AChesses> movedPiece, TWeakObjectPtr<AChesses> capturedPiece)
{
    SetChess(move.from.X, move.from.Y, movedPiece);
    SetChess(move.to.X, move.to.Y, capturedPiece);
}

TWeakObjectPtr<AChesses> UAI2P::GetChess(int32 x, int32 y) const
{
    if (x >= 0 && x < 10 && y >= 0 && y < 9)
    {
        return LocalAllChess[x][y];
    }
    return nullptr;
}

void UAI2P::SetChess(int32 x, int32 y, TWeakObjectPtr<AChesses> Chess)
{
    if (x >= 0 && x < 10 && y >= 0 && y < 9)
    {
        LocalAllChess[x][y] = Chess;
    }
}

bool UAI2P::IsValidPosition(int32 x, int32 y) const
{
    return x >= 0 && x < 10 && y >= 0 && y < 9;
}

bool UAI2P::IsInPalace(int32 x, int32 y, EChessColor color) const
{
    if (color == EChessColor::REDCHESS)
    {
        return x >= 0 && x <= 2 && y >= 3 && y <= 5;
    }
    else
    {
        return x >= 7 && x <= 9 && y >= 3 && y <= 5;
    }
}

TArray<FChessMove2P> UAI2P::GenerateAllMoves(EChessColor color)
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

TArray<FChessMove2P> UAI2P::GenerateMovesForChess(int32 x, int32 y, TWeakObjectPtr<AChesses> chess)
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

void UAI2P::GenerateJiangMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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

    // 添加将帅直接攻击的走法
    GenerateKingDirectAttackMoves(x, y, color, moves);
}

bool UAI2P::AreKingsFacingEachOther() const
{
    // 查找黑将和红帅的位置
    int32 blackKingX = -1, blackKingY = -1;
    int32 redKingX = -1, redKingY = -1;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG)
            {
                if (chess->GetColor() == EChessColor::BLACKCHESS)
                {
                    blackKingX = i;
                    blackKingY = j;
                }
                else
                {
                    redKingX = i;
                    redKingY = j;
                }
            }
        }
    }

    // 如果没找到将或帅，返回false
    if (blackKingX == -1 || redKingX == -1)
        return false;

    // 将帅必须在同一列（y坐标相同）
    if (blackKingY != redKingY)
        return false;

    // 检查中间是否有棋子阻挡
    int32 minX = FMath::Min(blackKingX, redKingX);
    int32 maxX = FMath::Max(blackKingX, redKingX);

    for (int32 x = minX + 1; x < maxX; x++)
    {
        TWeakObjectPtr<AChesses> chess = GetChess(x, blackKingY);
        if (chess.IsValid() && chess->GetType() != EChessType::EMPTY)
        {
            return false; // 中间有棋子阻挡
        }
    }

    return true; // 将帅面对面且中间无阻挡
}

int32 UAI2P::CountPiecesBetweenKings() const
{
    // 查找黑将和红帅的位置
    int32 blackKingX = -1, blackKingY = -1;
    int32 redKingX = -1, redKingY = -1;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG)
            {
                if (chess->GetColor() == EChessColor::BLACKCHESS)
                {
                    blackKingX = i;
                    blackKingY = j;
                }
                else
                {
                    redKingX = i;
                    redKingY = j;
                }
            }
        }
    }

    if (blackKingX == -1 || redKingX == -1 || blackKingY != redKingY)
        return -1;

    int32 count = 0;
    int32 minX = FMath::Min(blackKingX, redKingX);
    int32 maxX = FMath::Max(blackKingX, redKingX);

    for (int32 x = minX + 1; x < maxX; x++)
    {
        TWeakObjectPtr<AChesses> chess = GetChess(x, blackKingY);
        if (chess.IsValid() && chess->GetType() != EChessType::EMPTY)
        {
            count++;
        }
    }

    return count;
}

void UAI2P::GenerateKingDirectAttackMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 查找对方将/帅的位置
    EChessColor opponentColor = (color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 opponentKingX = -1, opponentKingY = -1;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> chess = GetChess(i, j);
            if (chess.IsValid() && chess->GetType() == EChessType::JIANG && chess->GetColor() == opponentColor)
            {
                opponentKingX = i;
                opponentKingY = j;
                break;
            }
        }
        if (opponentKingX != -1) break;
    }

    if (opponentKingX == -1) return;

    // 检查是否在同一列且中间无棋子
    if (y == opponentKingY)
    {
        int32 minX = FMath::Min(x, opponentKingX);
        int32 maxX = FMath::Max(x, opponentKingX);
        bool hasPieceBetween = false;

        for (int32 checkX = minX + 1; checkX < maxX; checkX++)
        {
            TWeakObjectPtr<AChesses> chess = GetChess(checkX, y);
            if (chess.IsValid() && chess->GetType() != EChessType::EMPTY)
            {
                hasPieceBetween = true;
                break;
            }
        }

        // 如果中间没有棋子，可以吃掉对方将/帅
        if (!hasPieceBetween)
        {
            moves.Add(FChessMove2P(Position(x, y), Position(opponentKingX, opponentKingY)));
        }
    }
}

void UAI2P::GenerateShiMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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

void UAI2P::GenerateXiangMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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
            if ((color == EChessColor::BLACKCHESS && newX >= 5) || (color == EChessColor::REDCHESS && newX <= 4))
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

void UAI2P::GenerateMaMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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

void UAI2P::GenerateJvMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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

void UAI2P::GeneratePaoMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
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

void UAI2P::GenerateBingMoves(int32 x, int32 y, EChessColor color, TArray<FChessMove2P>& moves) const
{
    // 兵/卒的移动方向
    TArray<TPair<int32, int32>> directions;

    if (color == EChessColor::BLACKCHESS)
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
