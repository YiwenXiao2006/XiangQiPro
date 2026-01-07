// Copyright 2026 Ultimate Player All Rights Reserved.

#include "AI2P.h"
#include "ChessBoard2P.h"
#include "XIANGQIPRO/Chess/Chesses.h"
#include <Kismet/GameplayStatics.h>

UAI2P::UAI2P()
{
}

void UAI2P::SetBoard(TWeakObjectPtr<UChessBoard2P> AIMove2P)
{
    board2P = AIMove2P;
}

bool UAI2P::IsMoveSuicidal(const FChessMove2P& Move, EChessColor AiColor)
{
    if (!board2P.IsValid()) return false;

    TWeakObjectPtr<AChesses> MovedChess = board2P->GetChess(Move.from.X, Move.from.Y);
    if (!MovedChess.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟走法
    TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
    board2P->MakeTestMove(Move);

    bool bIsSuicidal = false;

    // 检查移动后的棋子是否会被对方低价值棋子吃掉
    TArray<FChessMove2P> OpponentMoves = board2P->GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& OppMove : OpponentMoves) {
        if (OppMove.to.X == Move.to.X && OppMove.to.Y == Move.to.Y) {
            TWeakObjectPtr<AChesses> Attacker = board2P->GetChess(OppMove.from.X, OppMove.from.Y);
            if (Attacker.IsValid()) {
                int32 AttackerValue = GetPieceBaseValue(Attacker->GetType(), GetGamePhase(), OpponentColor);
                int32 MovedValue = GetPieceBaseValue(MovedChess->GetType(), GetGamePhase(), AiColor);

                // 如果被低价值棋子吃掉，且没有补偿，则是送子
                if (AttackerValue < MovedValue && !IsPieceRooted(Move.to.X, Move.to.Y, AiColor)) {
                    bIsSuicidal = true;
                    break;
                }
            }
        }
    }

    board2P->UndoTestMove(Move, Captured);
    return bIsSuicidal;
}

// 过滤车/炮的无效移动（自将、攻击有根且无收益、超出棋盘等）
TArray<FChessMove2P> UAI2P::FilterInvalidMoves(const TArray<FChessMove2P>& RawMoves, EChessColor AiColor, EChessType PieceType)
{
    TArray<FChessMove2P> ValidMoves;
    if (!board2P.IsValid()) return ValidMoves;

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
        TWeakObjectPtr<AChesses> TargetChess = board2P->GetChess(Move.to.X, Move.to.Y);
        if (TargetChess.IsValid())
        {
            // 目标是对方棋子：判断是否有根，且价值低于己方车/炮 → 无效移动
            if (TargetChess->GetColor() == OpponentColor)
            {
                bool bTargetRooted = IsPieceRooted(Move.to.X, Move.to.Y, OpponentColor);
                int32 TargetValue = GetPieceBaseValue(TargetChess->GetType(), GetGamePhase(), AiColor);
                int32 SelfValue = GetPieceBaseValue(PieceType, GetGamePhase(), AiColor);

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
            TWeakObjectPtr<AChesses> MovedChess = board2P->GetChess(Move.from.X, Move.from.Y);
            if (MovedChess.IsValid()) {
                // 模拟移动后检查是否能威胁对方棋子
                TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
                board2P->MakeTestMove(Move);

                TArray<FChessMove2P> ThreatMoves = board2P->GenerateMovesForChess(Move.to.X, Move.to.Y, MovedChess);
                for (const FChessMove2P& ThreatMove : ThreatMoves) {
                    TWeakObjectPtr<AChesses> ThreatTarget = board2P->GetChess(ThreatMove.to.X, ThreatMove.to.Y);
                    if (ThreatTarget.IsValid() && ThreatTarget->GetColor() == OpponentColor) {
                        bIsMeaningfulMove = true;
                        break;
                    }
                }

                board2P->UndoTestMove(Move, Captured);
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
    EChessGamePhase Phase = GetGamePhase();

    // 按游戏阶段优先级评估战术
    TArray<FTacticEvalResult> TacticResults;
    if (Phase == EChessGamePhase::Opening)
    {
        // 开局：优先中路突破
        TacticResults.Add(RecognizeZhongLuTuPo(AiColor));
        TacticResults.Add(RecognizeChenDiPao(AiColor));
    }
    else if (Phase == EChessGamePhase::Midgame)
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

    if (!board2P.IsValid()) return Result;

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
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::MA) continue;

            // 生成马的所有合法走法
            TArray<FChessMove2P> HorseMoves = board2P->GenerateMovesForChess(i, j, Chess);

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
                TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
                board2P->MakeTestMove(Move);
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
                    if (!board2P->GetChess(KM.X, KM.Y).IsValid()) KingMovableCount++;
                }
                board2P->UndoTestMove(Move, Captured);

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

    if (!board2P.IsValid()) return Result;

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
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::PAO) continue;

            // 生成炮的所有合法走法
            TArray<FChessMove2P> PaoMoves = board2P->GenerateMovesForChess(i, j, Chess);
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
                TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
                board2P->MakeTestMove(Move);
                // 检查同列是否有己方车/兵
                bool bHasChe = false;
                for (int32 k = 0; k < 10; k++)
                {
                    if (k == DiPaoX) continue;
                    TWeakObjectPtr<AChesses> C = board2P->GetChess(k, Move.to.Y);
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

                board2P->UndoTestMove(Move, Captured);

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

    if (!board2P.IsValid()) return Result;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 OppKingX, OppKingY;
    if (!GetKingPosition(OpponentColor, OppKingX, OppKingY)) return Result;

    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor) continue;

            EChessType Type = Chess->GetType();
            if (Type != EChessType::JV && Type != EChessType::PAO) continue;

            TArray<FChessMove2P> PieceMoves = board2P->GenerateMovesForChess(i, j, Chess);
            TArray<FChessMove2P> ValidMoves = FilterInvalidMoves(PieceMoves, AiColor, Type);

            for (const FChessMove2P& Move : ValidMoves)
            {
                // 不再简单判断Y=4，而是评估实际威胁

                int32 Feasibility = 0;

                // 1. 安全基础分
                if (IsCaptureSafe(Move, AiColor)) Feasibility += 30;

                // 重点评估对对方将帅的威胁
                if (Move.to.X == OppKingX || Move.to.Y == OppKingY) 
                {
                    Feasibility += 100; // 大幅增加威胁奖励
                }

                // 控制中路关键点
                if (Move.to.Y == 4) 
                {
                    Feasibility += 80;
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

    if (!board2P.IsValid()) return Result;

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
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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
        TArray<FChessMove2P> Che1RawMoves = board2P->GenerateMovesForChess(ChesLoc[c1].first, ChesLoc[c1].second, MyChes[c1]);
        TArray<FChessMove2P> Che1ValidMoves = FilterInvalidMoves(Che1RawMoves, AiColor, EChessType::JV);
        if (Che1ValidMoves.Num() == 0) continue; // 无有效移动，跳过

        for (const FChessMove2P& Move1 : Che1ValidMoves)
        {
            // 模拟第一辆车走法
            TWeakObjectPtr<AChesses> Captured1 = board2P->GetChess(Move1.to.X, Move1.to.Y);
            board2P->MakeTestMove(Move1);

            // 检查第二辆车是否能将军
            bool bHasSecondCheck = false;
            FChessMove2P SecondMove;
            for (int32 c2 = 0; c2 < MyChes.Num(); c2++)
            {
                if (c1 == c2) continue;

                // 【修改2】第二辆车也过滤无效移动
                TArray<FChessMove2P> Che2RawMoves = board2P->GenerateMovesForChess(ChesLoc[c2].first, ChesLoc[c2].second, MyChes[c2]);
                TArray<FChessMove2P> Che2ValidMoves = FilterInvalidMoves(Che2RawMoves, AiColor, EChessType::JV);
                if (Che2ValidMoves.Num() == 0) continue;

                for (const FChessMove2P& Move2 : Che2ValidMoves)
                {
                    TWeakObjectPtr<AChesses> Captured2 = board2P->GetChess(Move2.to.X, Move2.to.Y);
                    board2P->MakeTestMove(Move2);
                    bool bIsCheck = IsInCheck(OpponentColor);
                    board2P->UndoTestMove(Move2, Captured2);

                    if (bIsCheck)
                    {
                        bHasSecondCheck = true;
                        SecondMove = Move1;
                        break;
                    }
                }
                if (bHasSecondCheck) break;
            }

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

            board2P->UndoTestMove(Move1, Captured1);
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

    if (!board2P.IsValid()) return Result;

    EChessGamePhase Phase = GetGamePhase();
    if (Phase != EChessGamePhase::Endgame) return Result; // 仅残局执行

    // 遍历己方过河兵，优先推进
    int32 MaxFeasibility = 0;
    FChessMove2P BestMove;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != AiColor || Chess->GetType() != EChessType::BING) continue;

            // 判断是否过河
            bool bCrossed = (AiColor == EChessColor::REDCHESS) ? (i < 5) : (i >= 5);
            if (!bCrossed) continue;

            // 生成兵的合法走法（优先向前推进）
            TArray<FChessMove2P> BingMoves = board2P->GenerateMovesForChess(i, j, Chess);
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

// 判断走法是否为战术铺垫/执行
bool UAI2P::IsTacticMove(const FChessMove2P& Move, EChessColor AiColor, EChessTactic& OutTacticType)
{
    OutTacticType = EChessTactic::None;
    if (!board2P.IsValid()) return false;

    // 模拟走法，评估走法后是否能提升战术可行性
    FTacticEvalResult BeforeTactic = EvaluateBestTactic(AiColor);
    TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
    board2P->MakeTestMove(Move);
    FTacticEvalResult AfterTactic = EvaluateBestTactic(AiColor);
    board2P->UndoTestMove(Move, Captured);

    // 走法后战术可行性提升 → 是战术走法
    if (AfterTactic.FeasibilityScore > BeforeTactic.FeasibilityScore + 20)
    {
        OutTacticType = AfterTactic.TacticType;
        return true;
    }

    // 走法本身就是战术执行
    if (BeforeTactic.FeasibilityScore > 50 && Move == BeforeTactic.BestTacticMove)
    {
        OutTacticType = BeforeTactic.TacticType;
        return true;
    }

    return false;
}

// ===================== 新增：防守核心逻辑 =====================
// 预判对方是否有致命进攻（直接威胁将/帅）
bool UAI2P::IsOpponentHasLethalAttack(EChessColor AiColor)
{
    if (!board2P.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    // 生成对方所有合法走法，判断是否能直接吃将/帅
    TArray<FChessMove2P> OpponentMoves = board2P->GenerateAllMoves(OpponentColor);
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
    if (!board2P.IsValid()) return 0;

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
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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
                                    if (board2P->GetChess(Pao1.X, y).IsValid()) PieceCount++;
                                }
                            }
                            else // 竖线
                            {
                                int32 MinX = FMath::Min(Pao1.X, Pao2.X);
                                int32 MaxX = FMath::Max(Pao1.X, Pao2.X);
                                for (int32 x = MinX + 1; x < MaxX; x++)
                                {
                                    if (board2P->GetChess(x, Pao1.Y).IsValid()) PieceCount++;
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
                WeaknessScore -= 300; // 将门正前方/后方无保护
            else if (Point.X == KingX && Point.Y == KingY)
                WeaknessScore -= 500; // 将帅本身无保护
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
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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
    if (!board2P.IsValid()) return false;

    if ((int32) AIDifficulty < 1) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    TArray<FIntPoint> OpponentPaos;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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
    if (!board2P.IsValid()) return 0;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return 0;

    int32 ThreatScore = 0;

    // 收集对方所有炮的位置
    TArray<FIntPoint> OpponentCannons;
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::PAO) {
                OpponentCannons.Add(FIntPoint(i, j));
            }
        }
    }

    if (OpponentCannons.Num() < 2) return 0; // 没有双炮威胁

    // 检查每个炮与将帅的关系
    for (const FIntPoint& Cannon : OpponentCannons) {
        // 检查是否在同一条线上
        bool sameLine = (Cannon.X == KingX) || (Cannon.Y == KingY);
        if (!sameLine) continue;

        // 检查中间棋子数量
        int32 pieceCount = 0;
        if (Cannon.X == KingX) { // 同一横线
            int32 startY = FMath::Min(Cannon.Y, KingY) + 1;
            int32 endY = FMath::Max(Cannon.Y, KingY) - 1;
            for (int32 y = startY; y <= endY; y++) {
                if (board2P->GetChess(KingX, y).IsValid()) pieceCount++;
            }
        }
        else { // 同一竖线
            int32 startX = FMath::Min(Cannon.X, KingX) + 1;
            int32 endX = FMath::Max(Cannon.X, KingX) - 1;
            for (int32 x = startX; x <= endX; x++) {
                if (board2P->GetChess(x, KingY).IsValid()) pieceCount++;
            }
        }

        // 如果中间有1个棋子，这是炮的理想位置
        if (pieceCount == 1) {
            ThreatScore -= 800; // 单个炮威胁
        }
    }

    // 检查双炮配合威胁
    if (OpponentCannons.Num() >= 2) {
        // 检查是否有两个炮在同一条线上指向将帅
        for (int32 i = 0; i < OpponentCannons.Num(); i++) {
            for (int32 j = i + 1; j < OpponentCannons.Num(); j++) {
                FIntPoint cannon1 = OpponentCannons[i];
                FIntPoint cannon2 = OpponentCannons[j];

                // 检查是否在同一条线上且都指向将帅
                bool bothThreatenKing = false;
                if (cannon1.X == KingX && cannon2.X == KingX) {
                    // 两个炮在同一横线上威胁将帅
                    bothThreatenKing = true;
                }
                else if (cannon1.Y == KingY && cannon2.Y == KingY) {
                    // 两个炮在同一竖线上威胁将帅
                    bothThreatenKing = true;
                }

                if (bothThreatenKing) {
                    ThreatScore -= 2000; // 双炮协同威胁，严重扣分

                    // 如果是立即的双炮杀，返回极低分数
                    if (IsImmediateDoublePaoMate(AiColor)) {
                        return -10000; // 立即输棋的威胁
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
    if (!board2P.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 生成对方所有走法
    TArray<FChessMove2P> opponentMoves = board2P->GenerateAllMoves(OpponentColor);

    for (const FChessMove2P& move : opponentMoves) {
        // 模拟走法
        TWeakObjectPtr<AChesses> captured = board2P->GetChess(move.to.X, move.to.Y);
        board2P->MakeTestMove(move);

        // 检查是否形成双炮将军
        bool isDoubleCannonCheck = HasDoublePaoThreat(AiColor);

        board2P->UndoTestMove(move, captured);

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
    if (!board2P.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟对方的走法
    TWeakObjectPtr<AChesses> capturedByOpponent = board2P->GetChess(opponentMove.to.X, opponentMove.to.Y);
    board2P->MakeTestMove(opponentMove);

    // 获取将帅当前位置
    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY))
    {
        board2P->UndoTestMove(opponentMove, capturedByOpponent);
        return false;
    }

    // 生成将帅所有可能的移动
    TArray<FChessMove2P> kingMoves;
    TWeakObjectPtr<AChesses> king = board2P->GetChess(KingX, KingY);
    if (king.IsValid())
    {
        kingMoves = board2P->GenerateMovesForChess(KingX, KingY, king);
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
        TWeakObjectPtr<AChesses> capturedByKing = board2P->GetChess(escapeMove.to.X, escapeMove.to.Y);
        board2P->MakeTestMove(escapeMove);

        // 检查移动后是否仍然被将军
        bool stillInCheck = IsInCheck(AiColor);

        // 撤销将帅移动
        board2P->UndoTestMove(escapeMove, capturedByKing);

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
                TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
                if (chess.IsValid() && chess->GetColor() == OpponentColor && chess->GetType() == EChessType::PAO)
                {
                    // 检查这个炮是否能将军
                    TArray<FChessMove2P> cannonMoves = board2P->GenerateMovesForChess(i, j, chess);
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
                    TWeakObjectPtr<AChesses> attacker = board2P->GetChess(i, j);
                    if (attacker.IsValid() && attacker->GetColor() == AiColor)
                    {
                        TArray<FChessMove2P> attackMoves = board2P->GenerateMovesForChess(i, j, attacker);
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
                TWeakObjectPtr<AChesses> cannon = board2P->GetChess(i, j);
                if (cannon.IsValid() && cannon->GetColor() == OpponentColor && cannon->GetType() == EChessType::PAO)
                {
                    // 检查这个炮是否能将军
                    bool canCheck = false;
                    TArray<FChessMove2P> cannonMoves = board2P->GenerateMovesForChess(i, j, cannon);
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
                                    TWeakObjectPtr<AChesses> blocker = board2P->GetChess(x, y);
                                    if (blocker.IsValid() && blocker->GetColor() == AiColor && blocker->GetType() != EChessType::JIANG)
                                    {
                                        TArray<FChessMove2P> blockMoves = board2P->GenerateMovesForChess(x, y, blocker);
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
    board2P->UndoTestMove(opponentMove, capturedByOpponent);

    return canEscape;
}

// 检查走法是否能防御双炮
bool UAI2P::CanDefendAgainstDoublePao(const FChessMove2P& Move, EChessColor AiColor)
{
    if (!board2P.IsValid()) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 模拟走法
    TWeakObjectPtr<AChesses> captured = board2P->GetChess(Move.to.X, Move.to.Y);
    board2P->MakeTestMove(Move);

    // 检查走法后是否还面临严重的双炮威胁
    int32 threatAfterMove = EvaluateDoublePaoThreat(AiColor);

    board2P->UndoTestMove(Move, captured);

    // 如果威胁显著降低，认为这个走法有效
    return (threatAfterMove > -500); // 威胁降低到可接受水平
}

// 检查是否阻挡炮线
bool UAI2P::BlocksPaoLine(const FChessMove2P& Move, EChessColor AiColor)
{
    if (!board2P.IsValid()) return false;

    int32 KingX, KingY;
    if (!GetKingPosition(AiColor, KingX, KingY)) return false;

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 检查移动后的位置是否在对方炮和将帅之间的关键位置
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> chess = board2P->GetChess(i, j);
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

// 判断走法是否为有效防守（拦截致命进攻/补位弱点）
bool UAI2P::IsEffectiveDefenseMove(const FChessMove2P& Move, EChessColor AiColor)
{
    if (!board2P.IsValid()) return false;

    // 优先级1：拦截致命进攻
    if (IsOpponentHasLethalAttack(AiColor))
    {
        // 模拟走法后，检查对方是否还能致命进攻
        TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
        board2P->MakeTestMove(Move);
        bool bStillHasLethal = IsOpponentHasLethalAttack(AiColor);

        // 检查双炮威胁是否被化解
        bool bDoublePaoThreatRemoved = false;
        if (bStillHasLethal)
        {
            // 检查移动后是否破坏了对方的双炮线路
            bDoublePaoThreatRemoved = !HasDoublePaoThreat(AiColor);
        }

        board2P->UndoTestMove(Move, Captured);

        if (!bStillHasLethal || bDoublePaoThreatRemoved) return true;
    }

    // 优先级2：补位防守弱点（走法后防守弱点分值提升）
    int32 WeaknessBefore = EvaluateDefenseWeakness(AiColor);
    TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
    board2P->MakeTestMove(Move);
    int32 WeaknessAfter = EvaluateDefenseWeakness(AiColor);
    board2P->UndoTestMove(Move, Captured);

    if (WeaknessAfter > WeaknessBefore + 100) return true; // 弱点大幅改善

    // 优先级3：主动防守（卡位，限制对方进攻空间）
    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    TArray<FChessMove2P> OpponentMovesBefore = board2P->GenerateAllMoves(OpponentColor);
    board2P->MakeTestMove(Move);
    TArray<FChessMove2P> OpponentMovesAfter = board2P->GenerateAllMoves(OpponentColor);
    board2P->UndoTestMove(Move, Captured);

    if (OpponentMovesAfter.Num() < OpponentMovesBefore.Num() - 2) return true; // 对方走法减少，卡位有效

    return false;
}

// ===================== 新增：进攻核心逻辑 =====================
// 评估走法的进攻协同性（多棋子配合度）
int32 UAI2P::EvaluateAttackSynergy(const FChessMove2P& Move, EChessColor AiColor)
{
    if (!board2P.IsValid()) return 0;

    // 模拟走法
    TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
    board2P->MakeTestMove(Move);

    int32 SynergyScore = 0;
    TWeakObjectPtr<AChesses> MovedChess = board2P->GetChess(Move.to.X, Move.to.Y);
    if (!MovedChess.IsValid())
    {
        board2P->UndoTestMove(Move, Captured);
        return 0;
    }

    EChessType MovedType = MovedChess->GetType();
    int32 X = Move.to.X, Y = Move.to.Y;

    // 1. 车炮配合：车和炮在同一直线，中间有一个棋子（炮架）
    if (MovedType == EChessType::JV || MovedType == EChessType::PAO)
    {
        // 检查横向/纵向是否有己方炮/车
        for (int32 i = 0; i < 10; i++) // 纵向
        {
            if (i == X) continue;
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, Y);
            if (Chess.IsValid() && Chess->GetColor() == AiColor)
            {
                if ((MovedType == EChessType::JV && Chess->GetType() == EChessType::PAO) ||
                    (MovedType == EChessType::PAO && Chess->GetType() == EChessType::JV))
                {
                    // 检查中间是否有炮架（仅炮需要）
                    int32 PawnCount = 0;
                    int32 Start = FMath::Min(X, i), End = FMath::Max(X, i);
                    for (int32 j = Start + 1; j < End; j++)
                    {
                        if (board2P->GetChess(j, Y).IsValid()) PawnCount++;
                    }
                    if (MovedType == EChessType::PAO && PawnCount == 1) SynergyScore += 400;
                    if (MovedType == EChessType::JV) SynergyScore += 300;
                }
            }
        }

        for (int32 j = 0; j < 9; j++) // 横向
        {
            if (j == Y) continue;
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(X, j);
            if (Chess.IsValid() && Chess->GetColor() == AiColor)
            {
                if ((MovedType == EChessType::JV && Chess->GetType() == EChessType::PAO) ||
                    (MovedType == EChessType::PAO && Chess->GetType() == EChessType::JV))
                {
                    int32 PawnCount = 0;
                    int32 Start = FMath::Min(Y, j), End = FMath::Max(Y, j);
                    for (int32 k = Start + 1; k < End; k++)
                    {
                        if (board2P->GetChess(X, k).IsValid()) PawnCount++;
                    }
                    if (MovedType == EChessType::PAO && PawnCount == 1) SynergyScore += 400;
                    if (MovedType == EChessType::JV) SynergyScore += 300;
                }
            }
        }
    }

    // 2. 马炮配合：马和炮在相邻点位，形成夹击
    if (MovedType == EChessType::MA || MovedType == EChessType::PAO)
    {
        TArray<FIntPoint> Neighbors = {
            FIntPoint(X - 2, Y - 1), FIntPoint(X - 2, Y + 1), FIntPoint(X + 2, Y - 1), FIntPoint(X + 2, Y + 1),
            FIntPoint(X - 1, Y - 2), FIntPoint(X - 1, Y + 2), FIntPoint(X + 1, Y - 2), FIntPoint(X + 1, Y + 2)
        };
        for (const FIntPoint& Neighbor : Neighbors)
        {
            if (Neighbor.X < 0 || Neighbor.X >= 10 || Neighbor.Y < 0 || Neighbor.Y >= 9) continue;
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(Neighbor.X, Neighbor.Y);
            if (Chess.IsValid() && Chess->GetColor() == AiColor)
            {
                if ((MovedType == EChessType::MA && Chess->GetType() == EChessType::PAO) ||
                    (MovedType == EChessType::PAO && Chess->GetType() == EChessType::MA))
                {
                    SynergyScore += 300;
                }
            }
        }
    }

    // 撤销走法
    board2P->UndoTestMove(Move, Captured);

    return SynergyScore;
}

// 判断是否控制关键点位
bool UAI2P::IsControlKeyPoint(int32 X, int32 Y, EKeyChessPoint PointType, EChessColor Color)
{
    if (!board2P.IsValid()) return false;

    switch (PointType)
    {
    case EKeyChessPoint::Center:
        return Y == 4 && board2P->GetChess(X, Y).IsValid() && board2P->GetChess(X, Y)->GetColor() == Color;
    case EKeyChessPoint::KingGate:
    {
        int32 TargetKingX = (Color == EChessColor::REDCHESS) ? 9 : 0;
        return X == TargetKingX && Y == 4 && board2P->GetChess(X, Y).IsValid() && board2P->GetChess(X, Y)->GetColor() == Color;
    }
    case EKeyChessPoint::SoldierLine:
    {
        int32 TargetX = (Color == EChessColor::REDCHESS) ? 5 : 4;
        return X == TargetX && board2P->GetChess(X, Y).IsValid() && board2P->GetChess(X, Y)->GetColor() == Color;
    }
    case EKeyChessPoint::HorsePoint:
        return (X == 2 && Y == 1) || (X == 2 && Y == 7) || (X == 7 && Y == 1) || (X == 7 && Y == 7)
            && board2P->GetChess(X, Y).IsValid() && board2P->GetChess(X, Y)->GetColor() == Color;
    default:
        return false;
    }
}

// 识别对方防守弱点（返回弱点位置，无效返回(-1,-1)）
FIntPoint UAI2P::FindOpponentDefenseWeakness(EChessColor AiColor)
{
    if (!board2P.IsValid()) return FIntPoint(-1, -1);

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    int32 OppKingX, OppKingY; // 对手将的位置
    GetKingPosition(OpponentColor, OppKingX, OppKingY); 
    int32 AiKingX, AiKingY; // AI将的位置
    GetKingPosition(AiColor, AiKingX, AiKingY);

    // ===== 困难模式增加识别对方针对我方的杀招（AI优先防守关键位） =====
    if (AIDifficulty == EAI2PDifficulty::Hard)
    {
        // 1. 识别对方双车错杀招（两车分线攻击我方将帅）
        TArray<FIntPoint> OppJVList; // 对方车的位置列表
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::JV)
                {
                    OppJVList.Add(FIntPoint(i, j));
                }
            }
        }
        // 双车错判定：两车，且至少一车能直接将军，另一车辅助控线
        if (OppJVList.Num() >= 2)
        {
            bool bHasCheckmateJV = false;
            FIntPoint DefensePos(-1, -1);
            for (const FIntPoint& JVPos : OppJVList)
            {
                // 车能直接将军（同横线/竖线无遮挡）
                bool bIsSameRow = (JVPos.X == AiKingX) && (abs(JVPos.Y - AiKingY) > 0);
                bool bIsSameCol = (JVPos.Y == AiKingY) && (abs(JVPos.X - AiKingX) > 0);
                bool bNoBlock = true;
                if (bIsSameRow)
                {
                    // 检查横线是否有遮挡
                    int32 StartY = FMath::Min(JVPos.Y, AiKingY) + 1;
                    int32 EndY = FMath::Max(JVPos.Y, AiKingY) - 1;
                    for (int32 y = StartY; y <= EndY; y++)
                    {
                        if (board2P->GetChess(AiKingX, y).IsValid())
                        {
                            bNoBlock = false;
                            break;
                        }
                    }
                }
                else if (bIsSameCol)
                {
                    // 检查竖线是否有遮挡
                    int32 StartX = FMath::Min(JVPos.X, AiKingX) + 1;
                    int32 EndX = FMath::Max(JVPos.X, AiKingX) - 1;
                    for (int32 x = StartX; x <= EndX; x++)
                    {
                        if (board2P->GetChess(x, AiKingY).IsValid())
                        {
                            bNoBlock = false;
                            break;
                        }
                    }
                }
                if (bIsSameRow || bIsSameCol)
                {
                    if (bNoBlock)
                    {
                        bHasCheckmateJV = true;
                        // 防守位：将帅与车之间的关键空位/对方车的位置
                        DefensePos = (bIsSameRow) ? FIntPoint(AiKingX, (JVPos.Y + AiKingY) / 2) : FIntPoint((JVPos.X + AiKingX) / 2, AiKingY);
                        break;
                    }
                }
            }
            if (bHasCheckmateJV)
            {
                return DefensePos != FIntPoint(-1, -1) ? DefensePos : OppJVList[0]; // 优先防守双车错关键位
            }
        }

        // 2. 识别对方重炮（双炮）杀招（同线双炮，前炮架炮，后炮将军）
        TArray<FIntPoint> AiPAOList;
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::PAO)
                {
                    AiPAOList.Add(FIntPoint(i, j));
                }
            }
        }

        // 如果我方有炮，优先考虑破坏对方双炮线路
        if (AiPAOList.Num() > 0)
        {
            // 检查对方双炮威胁
            TArray<FIntPoint> OppPAOList;
            for (int32 i = 0; i < 10; i++)
            {
                for (int32 j = 0; j < 9; j++)
                {
                    TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                    if (Chess.IsValid() && Chess->GetColor() == OpponentColor && Chess->GetType() == EChessType::PAO)
                    {
                        OppPAOList.Add(FIntPoint(i, j));
                    }
                }
            }

            if (OppPAOList.Num() >= 2)
            {
                // 寻找可以破坏对方炮线的位置
                for (const FIntPoint& PaoPos : OppPAOList)
                {
                    // 尝试在炮的线路上插入棋子破坏炮架
                    if (PaoPos.X == OppKingX) // 横线威胁
                    {
                        for (int32 y = 0; y < 9; y++)
                        {
                            if (y == PaoPos.Y) continue;
                            FIntPoint BlockPos(OppKingX, y);
                            if (!board2P->GetChess(BlockPos.X, BlockPos.Y).IsValid())
                            {
                                return BlockPos; // 优先阻塞对方炮线
                            }
                        }
                    }
                    else if (PaoPos.Y == OppKingY) // 竖线威胁
                    {
                        for (int32 x = 0; x < 10; x++)
                        {
                            if (x == PaoPos.X) continue;
                            FIntPoint BlockPos(x, OppKingY);
                            if (!board2P->GetChess(BlockPos.X, BlockPos.Y).IsValid())
                            {
                                return BlockPos;
                            }
                        }
                    }
                }
            }
        }

        // ===== 识别己方可触发的杀招机会（AI优先进攻） =====
        // 1. 己方双车错机会：优先攻击对方将帅关键位
        TArray<FIntPoint> AiJVList;
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::JV)
                {
                    AiJVList.Add(FIntPoint(i, j));
                }
            }
        }
        if (AiJVList.Num() >= 2)
        {
            // 找能形成双车错的进攻位（对方将帅横线/竖线空位）
            TArray<FIntPoint> AttackPosList = {
                FIntPoint(OppKingX, 0), FIntPoint(OppKingX, 8), // 对方将帅横线两端
                FIntPoint(0, OppKingY), FIntPoint(9, OppKingY)  // 对方将帅竖线两端
            };
            for (const FIntPoint& AttackPos : AttackPosList)
            {
                if (AttackPos.X < 0 || AttackPos.X >= 10 || AttackPos.Y < 0 || AttackPos.Y >= 9) continue;
                if (!board2P->GetChess(AttackPos.X, AttackPos.Y).IsValid())
                {
                    return AttackPos; // 优先占双车错进攻位
                }
            }
        }

        // 2. 己方重炮机会：优先占炮架位/将军位
        if (AiPAOList.Num() >= 2)
        {
            // 找对方将帅同线的空位（作为炮架/将军位）
            for (const FIntPoint& PAOPos : AiPAOList)
            {
                bool bIsSameRow = (PAOPos.X == OppKingX);
                bool bIsSameCol = (PAOPos.Y == OppKingY);
                if (bIsSameRow || bIsSameCol)
                {
                    FIntPoint AttackPos = (bIsSameRow) ? FIntPoint(PAOPos.X, OppKingY) : FIntPoint(OppKingX, PAOPos.Y);
                    if (!board2P->GetChess(AttackPos.X, AttackPos.Y).IsValid())
                    {
                        return AttackPos; // 优先占重炮将军位
                    }
                }
            }
        }
    }

    // 获取当前游戏阶段
    EChessGamePhase CurrentPhase = GetGamePhase();

    // ===== 优化1：核心棋子按价值优先级（车>马>炮）查找无根目标 =====
    // 先找无根车（最高优先级）
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != OpponentColor) continue;
            if (Chess->GetType() == EChessType::JV && !IsPieceRooted(i, j, OpponentColor))
            {
                return FIntPoint(i, j); // 无根车是首要弱点
            }
        }
    }
    // 再找无根马
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != OpponentColor) continue;
            if (Chess->GetType() == EChessType::MA && !IsPieceRooted(i, j, OpponentColor))
            {
                return FIntPoint(i, j); // 无根马次之
            }
        }
    }
    // 最后找无根炮
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid() || Chess->GetColor() != OpponentColor) continue;
            if (Chess->GetType() == EChessType::PAO && !IsPieceRooted(i, j, OpponentColor))
            {
                return FIntPoint(i, j); // 无根炮再次之
            }
        }
    }

    // ===== 优化2：残局阶段，优先查找对方无根/无保护的过河兵 =====
    if (CurrentPhase == EChessGamePhase::Endgame)
    {
        for (int32 i = 0; i < 10; i++)
        {
            for (int32 j = 0; j < 9; j++)
            {
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                if (!Chess.IsValid() || Chess->GetColor() != OpponentColor) continue;
                if (Chess->GetType() == EChessType::BING)
                {
                    // 过河兵（判断：红兵过楚河/黑兵过汉界，复用棋盘逻辑）
                    bool bIsCrossed = (OpponentColor == EChessColor::REDCHESS && i < 5) || (OpponentColor == EChessColor::BLACKCHESS && i > 4);
                    if (bIsCrossed && !IsPieceRooted(i, j, OpponentColor))
                    {
                        return FIntPoint(i, j); // 残局无根过河兵是重要弱点
                    }
                }
            }
        }
    }

    // ===== 优化3：空门按威胁优先级排序（九宫核心>中路>边路） =====
    TArray<FIntPoint> WeakPoints = {
        FIntPoint(OppKingX, OppKingY - 1), // 将帅正前方（最致命）
        FIntPoint(OppKingX, 4),            // 中路核心
        FIntPoint(OppKingX, OppKingY + 1), // 将帅正后方
        FIntPoint(OppKingX - 1, OppKingY), // 将帅左侧
        FIntPoint(OppKingX + 1, OppKingY), // 将帅右侧
        FIntPoint(OppKingX, OppKingY)      // 将帅原位（最后考虑）
    };
    for (const FIntPoint& Point : WeakPoints)
    {
        if (Point.X < 0 || Point.X >= 10 || Point.Y < 0 || Point.Y >= 9) continue;
        if (!board2P->GetChess(Point.X, Point.Y).IsValid())
        {
            return Point; // 空门是次要弱点
        }
    }

    return FIntPoint(-1, -1);
}

// 新增：判断棋子是否有根（有己方保护）
bool UAI2P::IsPieceRooted(int32 X, int32 Y, EChessColor Color)
{
    if (!board2P.IsValid()) return false;

    TWeakObjectPtr<AChesses> TargetChess = board2P->GetChess(X, Y);
    if (!TargetChess.IsValid() || TargetChess->GetColor() != Color) return false;

    // 遍历己方所有棋子，判断是否能保护该位置
    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> ProtectChess = board2P->GetChess(i, j);
            if (!ProtectChess.IsValid() || ProtectChess->GetColor() != Color) continue;

            // 生成该保护棋子的所有合法走法，判断是否能走到目标位置（保护）
            TArray<FChessMove2P> ProtectMoves = board2P->GenerateMovesForChess(i, j, ProtectChess);
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

// 新增：评估吃子后的安全性（是否会被反吃）
bool UAI2P::IsCaptureSafe(const FChessMove2P& Move, EChessColor AttackerColor)
{
    if (!board2P.IsValid()) return false;

    // 1. 先判断是否是吃子走法
    TWeakObjectPtr<AChesses> CapturedChess = board2P->GetChess(Move.to.X, Move.to.Y);
    if (!CapturedChess.IsValid()) return true; // 非吃子，默认安全

    EChessColor DefenderColor = (AttackerColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 2. 模拟吃子
    board2P->MakeTestMove(Move);

    // 3. 检查对方是否能反吃该位置的棋子
    bool bIsSafe = true;
    TArray<FChessMove2P> DefenderMoves = board2P->GenerateAllMoves(DefenderColor);
    for (const FChessMove2P& DefMove : DefenderMoves)
    {
        if (DefMove.to.X == Move.to.X && DefMove.to.Y == Move.to.Y)
        {
            bIsSafe = false;
            break;
        }
    }

    // 4. 撤销模拟走法
    board2P->UndoTestMove(Move, CapturedChess);

    // 5. 若有根（己方保护），则即使能反吃也视为安全
    if (bIsSafe == false && IsPieceRooted(Move.to.X, Move.to.Y, AttackerColor))
    {
        bIsSafe = true;
    }

    return bIsSafe;
}

// 新增：判断棋子是否孤军深入（无支援）
bool UAI2P::IsPieceIsolated(int32 X, int32 Y, EChessColor Color)
{
    if (!board2P.IsValid()) return false;

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
        TWeakObjectPtr<AChesses> NeighborChess = board2P->GetChess(Neighbor.X, Neighbor.Y);
        if (NeighborChess.IsValid() && NeighborChess->GetColor() == Color)
        {
            return false; // 有支援，非孤军
        }
    }
    return true;
}

// 新增：检查核心棋子（车、马、炮）是否被攻击
bool UAI2P::IsKeyPieceUnderAttack(EChessColor Color)
{
    if (!board2P.IsValid()) return false;

    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    TArray<FChessMove2P> OpponentMoves = board2P->GenerateAllMoves(OpponentColor);

    // 遍历己方核心棋子，判断是否被攻击
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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

// 缓存将/帅位置（优化：避免重复遍历棋盘）
bool UAI2P::GetKingPosition(EChessColor Color, int32& OutX, int32& OutY)
{
    OutX = -1;
    OutY = -1;
    if (!board2P.IsValid()) return false;

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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

// 校验：走指定棋步后，己方是否会被将军（自将）
bool UAI2P::IsInCheckAfterMove(const FChessMove2P& Move, EChessColor SelfColor)
{
    if (!board2P.IsValid()) return false;

    // 1. 保存被吃棋子（用于回滚）
    TWeakObjectPtr<AChesses> CapturedChess = board2P->GetChess(Move.to.X, Move.to.Y);
    // 2. 模拟走棋
    board2P->MakeTestMove(Move);
    // 3. 检查走棋后己方是否被将军
    bool bIsSelfCheck = IsInCheck(SelfColor);
    // 4. 回滚棋盘
    board2P->UndoTestMove(Move, CapturedChess);

    return bIsSelfCheck;
}

// 校验：当前局面下，我方下一步是否能直接吃掉对方将
bool UAI2P::CanCaptureGeneralInNextStep(EChessColor SelfColor)
{
    if (!board2P.IsValid()) return false;

    EChessColor OppColor = (SelfColor == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;
    // 1. 找到对方将的位置
    FVector2D GeneralPos = FVector2D(-1, -1);
    for (int32 X = 0; X < 10; X++)
    {
        for (int32 Y = 0; Y < 9; Y++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(X, Y);
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
    TArray<FChessMove2P> SelfMoves = board2P->GenerateAllMoves(SelfColor);
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
    if (!board2P.IsValid()) return false;

    int32 KingX, KingY;
    if (!GetKingPosition(Color, KingX, KingY)) return false;

    EChessColor OpponentColor = (Color == EChessColor::BLACKCHESS) ?
        EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    TArray<FChessMove2P> OpponentMoves = board2P->GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& Move : OpponentMoves) 
    {
        if (Move.to.X == KingX && Move.to.Y == KingY) 
        {
            return true;
        }
    }
    return false;
}

// 优化后的IsBlockingAttack（减少重复计算）
bool UAI2P::IsBlockingAttack(const FChessMove2P& move, EChessColor color)
{
    if (!board2P.IsValid()) return false;

    // 缓存将/帅位置（避免重复遍历）
    int32 KingX, KingY;
    if (!GetKingPosition(color, KingX, KingY)) return false;

    EChessColor OpponentColor = (color == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

    // 第一步：计算走法前是否被攻击（仅生成一次对方走法）
    bool canAttackKingBeforeMove = false;
    TArray<FChessMove2P> OpponentMoves = board2P->GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& OppMove : OpponentMoves)
    {
        if (OppMove.to.X == KingX && OppMove.to.Y == KingY)
        {
            canAttackKingBeforeMove = true;
            break;
        }
    }
    if (!canAttackKingBeforeMove) return false; // 走法前未被攻击，无需判断

    // 模拟走法
    TWeakObjectPtr<AChesses> CapturedChess = board2P->GetChess(move.to.X, move.to.Y);
    board2P->MakeTestMove(move);

    // 计算走法后是否被攻击（复用OpponentColor，无需重复遍历将/帅）
    bool canAttackKingAfterMove = false;
    TArray<FChessMove2P> NewOpponentMoves = board2P->GenerateAllMoves(OpponentColor);
    for (const FChessMove2P& OppMove : NewOpponentMoves)
    {
        if (OppMove.to.X == KingX && OppMove.to.Y == KingY)
        {
            canAttackKingAfterMove = true;
            break;
        }
    }

    // 撤销走法
    board2P->UndoTestMove(move, CapturedChess);

    // 走法前被攻击、走法后未被攻击 → 阻挡成功
    return !canAttackKingAfterMove;
}

// 获取游戏阶段
EChessGamePhase UAI2P::GetGamePhase()
{
    if (!board2P.IsValid()) return EChessGamePhase::Midgame;

    // 统计剩余棋子数量（简化逻辑：可根据实际需求调整）
    int32 TotalPieces = 0;
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            if (board2P->GetChess(i, j).IsValid()) TotalPieces++;
        }
    }

    if (TotalPieces >= 20) return EChessGamePhase::Opening;   // 开局（棋子多）
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
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
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
                TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
                if (Chess.IsValid() && Chess->GetColor() == AiColor && Chess->GetType() == EChessType::PAO)
                {
                    PAOCount++;
                }
            }
        }
        // 双炮时价值+150，单炮在残局-20
        if (PAOCount >= 2) return VALUE_PAO + 150;
        return Phase == EChessGamePhase::Endgame ? VALUE_PAO - 20 : VALUE_PAO;
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
            PosValue += 30;
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

                TWeakObjectPtr<AChesses> Target = board2P->GetChess(X, targetY);
                if (Target.IsValid() && Target->GetColor() != Color)
                {
                    // 检查中间是否有炮架
                    int32 PawnCount = 0;
                    int32 startY = FMath::Min(targetY, Y) + 1;
                    int32 endY = FMath::Max(targetY, Y) - 1;
                    for (int32 yy = startY; yy <= endY; yy++)
                    {
                        if (board2P->GetChess(X, yy).IsValid()) PawnCount++;
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

                    TWeakObjectPtr<AChesses> Target = board2P->GetChess(targetX, Y);
                    if (Target.IsValid() && Target->GetColor() != Color)
                    {
                        int32 PawnCount = 0;
                        int32 startX = FMath::Min(targetX, X) + 1;
                        int32 endX = FMath::Max(targetX, X) - 1;
                        for (int32 xx = startX; xx <= endX; xx++)
                        {
                            if (board2P->GetChess(xx, Y).IsValid()) PawnCount++;
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
            if (board2P->GetChess(X, Y).IsValid())
            {
                ThreatMoves = board2P->GenerateMovesForChess(X, Y, board2P->GetChess(X, Y));
            }

            for (const FChessMove2P& Move : ThreatMoves)
            {
                TWeakObjectPtr<AChesses> Target = board2P->GetChess(Move.to.X, Move.to.Y);
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

    // 通用评估：孤军和有根
    if (IsPieceIsolated(X, Y, Color))
        PosValue += PENALTY_ISOLATED_PIECE;
    if (IsPieceRooted(X, Y, Color))
        PosValue += BONUS_ROOTED_PIECE;

    // 新增：控制关键点位加分（但炮的控制需要更严格的条件）
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
            if (board2P->GetChess(X, Y).IsValid())
            {
                AttackMoves = board2P->GenerateMovesForChess(X, Y, board2P->GetChess(X, Y));
            }

            for (const FChessMove2P& Move : AttackMoves)
            {
                TWeakObjectPtr<AChesses> Target = board2P->GetChess(Move.to.X, Move.to.Y);
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
    if (!board2P.IsValid()) return 0;

    // === 新增：优先检测双炮将军威胁 ===
    int32 DoubleCannonThreat = EvaluateDoublePaoThreat(AiColor);
    if (DoubleCannonThreat < -5000) {
        // 如果存在致命双炮威胁，直接返回极低分数
        return DoubleCannonThreat;
    }

    EChessColor OpponentColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
    EChessGamePhase Phase = GetGamePhase();

    int32 TotalScore = 0;

    // 叠加双炮威胁评估
    TotalScore += DoubleCannonThreat;

    // 检查当前局面下是否能直接吃掉对方将
    TArray<FChessMove2P> AllMoves = board2P->GenerateAllMoves(AiColor);
    for (const FChessMove2P& Move : AllMoves)
    {
        TWeakObjectPtr<AChesses> TargetChess = board2P->GetChess(Move.to.X, Move.to.Y);
        if (TargetChess.IsValid() && TargetChess->GetType() == EChessType::JIANG &&
            TargetChess->GetColor() == OpponentColor)
        {
            // 直接返回最高分，确保AI选择这个走法
            return VALUE_JIANG * 2; // 吃将的价值应该远高于其他
        }
    }
    
    if (CanCaptureGeneralInNextStep(AiColor))
    {
        return BONUS_CHECK; // 直接返回最高权重，确保AI优先选择
    }
    
    // 自将惩罚：如果己方被将军，直接扣致命分
    if (IsInCheck(AiColor))
    {
        TotalScore += PENALTY_IN_CHECK; // PENALTY_IN_CHECK 已定义为 -10000
        return TotalScore; // 自将时直接返回最低分，优先级最高
    }

    // 1. 基础棋子价值+位置价值
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (!Chess.IsValid()) continue;

            EChessColor ChessColor = Chess->GetColor();
            EChessType ChessType = Chess->GetType();

            int32 PieceValue = GetPieceBaseValue(ChessType, Phase, AiColor) + GetPiecePositionValue(ChessType, ChessColor, i, j);
            TotalScore += (ChessColor == AiColor) ? PieceValue : -PieceValue;
        }
    }

    // 2. 防守评估
    if (IsOpponentHasLethalAttack(AiColor)) TotalScore += PENALTY_DEFENSE_WEAK;
    TotalScore += EvaluateDefenseWeakness(AiColor);

    // 3. 进攻评估
    FIntPoint OppWeakPoint = FindOpponentDefenseWeakness(AiColor);
    if (OppWeakPoint.X != -1 && OppWeakPoint.Y != -1)
    {
        TArray<FChessMove2P> AiMoves = board2P->GenerateAllMoves(AiColor);
        for (const FChessMove2P& Move : AiMoves)
        {
            if (Move.to.X == OppWeakPoint.X && Move.to.Y == OppWeakPoint.Y)
            {
                TotalScore += BONUS_ATTACK_WEAKNESS;
                break;
            }
        }
    }

    // 增加进攻积极性评估
    int32 AggressivenessScore = 0;

    // 计算己方棋子在前场的数量
    int32 FrontlinePieces = 0;
    for (int32 i = 0; i < 10; i++) 
    {
        for (int32 j = 0; j < 9; j++) 
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == AiColor) 
            {
                // 在前场（对方半场）的棋子
                if ((AiColor == EChessColor::REDCHESS && i <= 4) ||
                    (AiColor == EChessColor::BLACKCHESS && i >= 5)) 
                {
                    FrontlinePieces++;

                    // 根据棋子类型给予不同的进攻奖励
                    switch (Chess->GetType()) 
                    {
                    case EChessType::JV: AggressivenessScore += 50; break;
                    case EChessType::PAO: AggressivenessScore += 40; break;
                    case EChessType::MA: AggressivenessScore += 30; break;
                    case EChessType::BING: AggressivenessScore += 20; break;
                    }
                }
            }
        }
    }

    TotalScore += AggressivenessScore;

    // 4. 将军/被将军
    if (IsInCheck(OpponentColor)) TotalScore += BONUS_CHECK;
    if (IsInCheck(AiColor)) TotalScore += PENALTY_IN_CHECK;

    // === 新增：战术奖励（核心）===
    FTacticEvalResult BestTactic = EvaluateBestTactic(AiColor);
    if (BestTactic.FeasibilityScore > 50)
    {
        // 基础战术奖励
        TotalScore += BONUS_TACTIC_EXECUTE;
        // 不同战术额外奖励
        switch (BestTactic.TacticType)
        {
        case EChessTactic::WoCaoMa: TotalScore += BONUS_WOCAOMA; break;
        case EChessTactic::ChenDiPao: TotalScore += BONUS_CHENDIPAO; break;
        case EChessTactic::ZhongLuTuPo: TotalScore += BONUS_ZHONGLUTUPO; break;
        default: break;
        }
    }

    // 新增：安全性评估
    int32 SafetyScore = 0;

    // 检查己方棋子是否处于危险中
    for (int32 i = 0; i < 10; i++) {
        for (int32 j = 0; j < 9; j++) {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (Chess.IsValid() && Chess->GetColor() == AiColor) {
                // 检查棋子是否无保护且被攻击
                if (!IsPieceRooted(i, j, AiColor)) {
                    // 检查是否被对方攻击
                    EChessColor OppColor = (AiColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;
                    TArray<FChessMove2P> OppMoves = board2P->GenerateAllMoves(OppColor);
                    for (const FChessMove2P& Move : OppMoves) {
                        if (Move.to.X == i && Move.to.Y == j) {
                            SafetyScore -= 100; // 无保护被攻击，扣分
                            break;
                        }
                    }
                }
            }
        }
    }
    TotalScore += SafetyScore;

    return TotalScore;
}

// 走法排序（优先搜索高价值走法，提升α-β剪枝效率）
void UAI2P::SortMoves(TArray<FChessMove2P>& Moves, EChessColor Color)
{
    if (Moves.Num() <= 1) return;


    // ===== 分离「吃将走法」「将军且能吃将走法」「普通走法」=====
    for (const FChessMove2P& Move : Moves)
    {
        // 1. 识别「直接吃将」的走法
        TWeakObjectPtr<AChesses> TargetChess = board2P->GetChess(Move.to.X, Move.to.Y);
        if (TargetChess.IsValid() && TargetChess->GetType() == EChessType::JIANG)
        {
            Moves.Empty();
            Moves.Add(Move);
            return;
        }
    }

    EChessColor OppColor = (Color == EChessColor::REDCHESS) ? EChessColor::BLACKCHESS : EChessColor::REDCHESS;

    // 先过滤掉「走后自将」的非法走法
    TArray<FChessMove2P> ValidMoves;
    for (const FChessMove2P& Move : Moves)
    {
        if (IsInCheckAfterMove(Move, Color))
        {
            continue;
        }

        // 检查是否会导致送子
        if (IsMoveSuicidal(Move, Color)) {
            // 只有在高价值补偿时才不过滤（比如将军或吃大子）
            bool bHasHighValueCompensation = false;

            // 检查是否将军
            if (IsInCheckAfterMove(Move, OppColor)) {
                bHasHighValueCompensation = true;
            }

            // 检查是否吃子
            TWeakObjectPtr<AChesses> Target = board2P->GetChess(Move.to.X, Move.to.Y);
            if (Target.IsValid()) {
                int32 TargetValue = GetPieceBaseValue(Target->GetType(), GetGamePhase(), Color);
                TWeakObjectPtr<AChesses> Mover = board2P->GetChess(Move.from.X, Move.from.Y);
                int32 MoverValue = GetPieceBaseValue(Mover->GetType(), GetGamePhase(), Color);

                // 如果吃的子价值 >= 移动的子价值，可以冒险
                if (TargetValue >= MoverValue) {
                    bHasHighValueCompensation = true;
                }
            }

            if (!bHasHighValueCompensation) {
                continue; // 过滤无谓的送子
            }
        }

        ValidMoves.Add(Move);
    }
    Moves = ValidMoves;

    if (ValidMoves.Num() == 0)
    {
        Moves = ValidMoves;
        return;
    }

    TArray<FChessMove2P> CheckCanCaptureGeneralMoves; // 将军且下一步能吃将的走法
    TArray<FChessMove2P> NormalMoves; // 普通走法

    for (const FChessMove2P& Move : Moves)
    {
        // 2. 识别「将军且下一步能吃将」的走法
        if (IsInCheckAfterMove(Move, OppColor)) // 走该步后对方被将军
        {
            // 模拟走棋后，检查是否能直接吃将
            TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
            board2P->MakeTestMove(Move);
            // 生成对方被将军后的所有合法走法，判断我方下一步是否能吃将
            bool bCanCaptureGeneralNext = CanCaptureGeneralInNextStep(Color);
            board2P->UndoTestMove(Move, Captured);

            if (bCanCaptureGeneralNext)
            {
                CheckCanCaptureGeneralMoves.Add(Move);
                continue;
            }
        }

        // 3. 其他为普通走法
        NormalMoves.Add(Move);
    }

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
        RANDOM_THRESHOLD = 5;  // 窄范围
        RANDOM_TWEAK_RANGE = 1;  // 微小扰动
        break;
    }

    // ===== 重新排序：将军且能吃将 > 普通走法 =====
    // 1. 先加将军且能吃将的走法
    TArray<FChessMove2P> SortedMoves = CheckCanCaptureGeneralMoves;
    // 2. 普通走法按原有评分排序后追加
    TArray<FMoveScore> MoveScores;

    // === 新增：优先处理双炮威胁防御 ===
    int32 doublePaoThreat = EvaluateDoublePaoThreat(Color);
    bool hasSeriousPaoThreat = (doublePaoThreat < -1000);

    for (const FChessMove2P& Move : NormalMoves)
    {
        int32 Score = 0;
        EChessTactic TacticType;

        // 最高优先级：致命威胁
        if (CanCaptureGeneralInNextStep(Color)) {
            Score += 5000;
        }


        // 高优先级：双炮威胁防御
        if (hasSeriousPaoThreat) {
            // 检查这个走法是否能缓解双炮威胁
            if (CanDefendAgainstDoublePao(Move, Color)) {
                Score += 1500; // 防御双炮的高奖励
            }

            // 移动将帅避开炮线
            TWeakObjectPtr<AChesses> movedPiece = board2P->GetChess(Move.from.X, Move.from.Y);
            if (movedPiece.IsValid() && movedPiece->GetType() == EChessType::JIANG) {
                Score += 800; // 将帅移动避开威胁
            }

            // 吃对方的炮
            TWeakObjectPtr<AChesses> target = board2P->GetChess(Move.to.X, Move.to.Y);
            if (target.IsValid() && target->GetType() == EChessType::PAO && target->GetColor() != Color) {
                Score += 1200; // 吃炮的高奖励
            }

            // 阻挡炮线
            if (BlocksPaoLine(Move, Color)) {
                Score += 600; // 阻挡炮线奖励
            }
        }

        // 高优先级：战术执行
        if (IsTacticMove(Move, Color, TacticType)) {
            Score += BONUS_TACTIC_EXECUTE;
            switch (TacticType) {
            case EChessTactic::WoCaoMa: Score += BONUS_WOCAOMA; break;
            case EChessTactic::ChenDiPao: Score += BONUS_CHENDIPAO; break;
            case EChessTactic::ZhongLuTuPo: Score += BONUS_ZHONGLUTUPO; break;
            }
        }

        // 中高优先级：安全吃子
        bool bIsCapture = board2P->GetChess(Move.to.X, Move.to.Y).IsValid();
        if (bIsCapture) {
            if (IsCaptureSafe(Move, Color)) {
                Score += BONUS_SAFE_CAPTURE;

                // 根据吃的棋子价值额外加分
                TWeakObjectPtr<AChesses> Target = board2P->GetChess(Move.to.X, Move.to.Y);
                if (Target.IsValid()) {
                    int32 TargetValue = GetPieceBaseValue(Target->GetType(), GetGamePhase(), Color);
                    Score += TargetValue / 10; // 按价值比例加分
                }
            }
        }

        // 中优先级：进攻协同和控点
        Score += EvaluateAttackSynergy(Move, Color);

        if (IsControlKeyPoint(Move.to.X, Move.to.Y, EKeyChessPoint::Center, Color) ||
            IsControlKeyPoint(Move.to.X, Move.to.Y, EKeyChessPoint::KingGate, Color)) {
            Score += BONUS_CONTROL_KEY_POINT;
        }

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

        // 炮移动的特殊处理：惩罚无意义的炮移动
        TWeakObjectPtr<AChesses> MovedChess = board2P->GetChess(Move.from.X, Move.from.Y);
        if (MovedChess.IsValid() && MovedChess->GetType() == EChessType::PAO) {
            bool bIsMeaningful = false;

            // 检查移动后是否有实际威胁
            TWeakObjectPtr<AChesses> Captured = board2P->GetChess(Move.to.X, Move.to.Y);
            board2P->MakeTestMove(Move);

            TArray<FChessMove2P> ThreatMoves = board2P->GenerateMovesForChess(Move.to.X, Move.to.Y, MovedChess);
            for (const FChessMove2P& ThreatMove : ThreatMoves) {
                TWeakObjectPtr<AChesses> ThreatTarget = board2P->GetChess(ThreatMove.to.X, ThreatMove.to.Y);
                if (ThreatTarget.IsValid() && ThreatTarget->GetColor() == OppColor) {
                    bIsMeaningful = true;
                    break;
                }
            }

            board2P->UndoTestMove(Move, Captured);

            if (!bIsMeaningful) {
                Score -= 50; // 适度惩罚无意义移动
            }
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

    // 重新传值
    Moves = SortedMoves;
}

// Zobrist哈希生成（简化版，需根据棋盘规则完善）
uint64 UAI2P::GenerateZobristKey()
{
    if (!board2P.IsValid()) return 0;

    uint64 Key = 0;
    // 示例：简单哈希（可替换为标准Zobrist哈希表）
    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            TWeakObjectPtr<AChesses> Chess = board2P->GetChess(i, j);
            if (Chess.IsValid())
            {
                Key ^= (uint64)(i * 9 + j) * (uint64)Chess->GetColor() * (uint64)Chess->GetType();
            }
        }
    }
    return Key;
}

// α-β剪枝核心搜索
int32 UAI2P::AlphaBetaSearch(int32 Depth, int32 Alpha, int32 Beta, EChessColor CurrentColor, bool IsMaximizingPlayer)
{
    uint64 ZobristKey = GenerateZobristKey();
    EChessColor OpponentColor = (CurrentColor == EChessColor::BLACKCHESS) ? EChessColor::REDCHESS : EChessColor::BLACKCHESS;

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
    if (Depth <= 0) {
        int32 eval = EvaluateBoard(CurrentColor);

        // === 新增：如果面临立即的双炮杀，提前返回极低分数 ===
        if (IsImmediateDoublePaoMate(CurrentColor)) {
            return IsMaximizingPlayer ? -10000 : 10000;
        }

        return eval;
    }

    // 3. 生成并排序走法（优先高价值走法，提升剪枝效率）
    TArray<FChessMove2P> Moves = board2P->GenerateAllMoves(CurrentColor);
    SortMoves(Moves, CurrentColor);
    if (Moves.Num() == 0)
    {
        // 无子可走：如果是被将军则判负，否则和棋
        return IsInCheck(CurrentColor) ? -VALUE_JIANG : 0;
    }

    // 仅一步可走
    if (Moves.Num() == 1)
    {
        return VALUE_JIANG * 10;
    }

    int32 BestValue = IsMaximizingPlayer ? INT_MIN : INT_MAX;
    FChessMove2P BestMove;

    // 4. 遍历所有走法，执行α-β剪枝
    for (const FChessMove2P& Move : Moves)
    {
        // 再次兜底校验,防止漏过滤的自将走法
        if (IsInCheckAfterMove(Move, CurrentColor))
        {
            continue;
        }
        // 模拟走法
        TWeakObjectPtr<AChesses> CapturedChess = board2P->GetChess(Move.to.X, Move.to.Y);
        board2P->MakeTestMove(Move);

        // 递归搜索下一层
        int32 CurrentValue = AlphaBetaSearch(Depth - 1, Alpha, Beta, OpponentColor, !IsMaximizingPlayer);

        // 撤销走法
        board2P->UndoTestMove(Move, CapturedChess);

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

        // 超时
        if (Clock.GetElapsedMilliseconds() > MaxTime) break;
    }

    // 5. 更新置换表
    FTranspositionEntry NewEntry;
    NewEntry.ZobristKey = ZobristKey;
    NewEntry.Depth = Depth;
    NewEntry.Value = BestValue;
    NewEntry.BestMove = BestMove;

    if (BestValue <= Alpha) NewEntry.Flag = ETranspositionFlag::Alpha;
    else if (BestValue >= Beta) NewEntry.Flag = ETranspositionFlag::Beta;
    else NewEntry.Flag = ETranspositionFlag::Exact;

    TranspositionTable.Add(ZobristKey, NewEntry);

    return BestValue;
}

// 迭代加深搜索
int32 UAI2P::IterativeDeepeningSearch(int32 MaxDepth, EChessColor AiColor)
{
    ClearTranspositionTable(); // 每次迭代前清空置换表

    int32 BestScore = 0;
    FChessMove2P BestMove;

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
        if (Clock.GetElapsedMilliseconds() > MaxTime) break;
    }

    return BestScore;
}

// 获取AI最优走法（对外接口）
FChessMove2P UAI2P::GetBestMove(EChessColor InAiColor, EAI2PDifficulty InDifficulty, int32 InMaxTime)
{
    if (!board2P.IsValid()) return FChessMove2P();

    AIDifficulty = InDifficulty;
    MaxTime = InMaxTime;
    Clock.Start();

    // 执行迭代加深搜索
    IterativeDeepeningSearch(GetSearchDepth(AIDifficulty), InAiColor);

    // 从置换表中获取最优走法
    uint64 CurrentKey = GenerateZobristKey();
    if (TranspositionTable.Contains(CurrentKey))
    {
        return TranspositionTable[CurrentKey].BestMove;
    }

    // 兜底：返回第一个合法走法
    TArray<FChessMove2P> Moves = board2P->GenerateAllMoves(InAiColor);
    return Moves.Num() > 0 ? Moves[0] : FChessMove2P();
}

void UAI2P::StopThinkingImmediately()
{
    MaxTime = 0;
}
