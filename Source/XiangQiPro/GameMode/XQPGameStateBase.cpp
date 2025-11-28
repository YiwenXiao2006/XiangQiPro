// Copyright 2026 Ultimate Player All Rights Reserved.

#include "XQPGameStateBase.h"
#include "Async/Async.h"

#include "../Chess/Chesses.h"
#include "../GameObject/AI2P.h"
#include "../GameObject/ChessBoard2P.h"
#include "../GameObject/ChessBoard2PActor.h"

#include "../UI/UI_Battle2P_Base.h"

#include "../Util/Logger.h"
#include "../Util/ChessMove.h"
#include "../Util/ChessInfo.h"
#include "../Util/AsyncWorker.h"

void AXQPGameStateBase::UpdateScore()
{
    score1 = AI2P->Evaluate(EChessColor::RED);
    score2 = AI2P->Evaluate(EChessColor::BLACK);
    if (HUD2P.IsValid())
    {
        HUD2P->UpdateScore(score1, score2);
    }
    else
    {
        ULogger::LogWarning(TEXT("AXQPGameStateBase::UpdateScore: HUD2P is nullptr."));
    }
}

AXQPGameStateBase::AXQPGameStateBase() : Super(), battleType(EBattleType::P2_AI)
{
}

void AXQPGameStateBase::BeginPlay()
{
	Super::BeginPlay();
}

void AXQPGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AIAsync)
    {
        if (AIAsync->IsRunning())
        {
            AI2P->StopThinkingImmediately(); // 停止AI工作
            AIAsync->StopAsyncWork(); // 停止异步任务
        }
    }

    // 释放掉UObject对象
    if (board2P.IsValid())
        board2P->RemoveFromRoot();
    board2P.Reset();

    if (AI2P.IsValid())
        AI2P->RemoveFromRoot();
    AI2P.Reset();

    board2PActor.Reset();

    Super::EndPlay(EndPlayReason);
}

void AXQPGameStateBase::ShowSettingPoint2P(TArray<FChessMove2P> Moves, TWeakObjectPtr<AChesses> Target)
{
    DismissSettingPoint2P();
    board2P->ShowSettingPoint2P(Moves, Target);
}

void AXQPGameStateBase::DismissSettingPoint2P()
{
    board2P->DismissSettingPoint2P();
}

TWeakObjectPtr<UChessBoard2P> AXQPGameStateBase::GetChessBoard2P()
{
    return board2P;
}

EBattleType AXQPGameStateBase::GetBattleType()
{
    return battleType;
}

EPlayerTag AXQPGameStateBase::GetBattleTurn()
{
    return battleTurn;
}

void AXQPGameStateBase::SetHUD2P(TWeakObjectPtr<UUI_Battle2P_Base> hud2P)
{
    HUD2P = hud2P;
}

int32 AXQPGameStateBase::GetScore1() const
{
    return score1;
}

int32 AXQPGameStateBase::GetScore2() const
{
    return score2;
}

int32 AXQPGameStateBase::GetScore3() const
{
    return score3;
}

void AXQPGameStateBase::Start2PGame(TWeakObjectPtr<AChessBoard2PActor> InBoard2PActor)
{
    board2PActor = InBoard2PActor;
    if (board2PActor.IsValid())
    {
        if (!board2P.IsValid())
        {
            board2P = NewObject<UChessBoard2P>(this);
            board2P->AddToRoot();
        }

        board2P->InitializeBoard(board2PActor); // 初始化棋盘
        board2PActor->GenerateChesses(board2P); // 棋盘Actor生成所有象棋并对其初始化

        if (!AI2P.IsValid())
        {
            AI2P = NewObject<UAI2P>(this);
            AI2P->AddToRoot();
        }

        AI2P->SetDepth(4); // 搜索深度
        AI2P->SetBoard(board2P); // 棋盘状态
    }
    else
    {
        ULogger::LogError(TEXT("AXQPGameStateBase::Start2PGame: Can't start two players game, because board2P actor is nullptr!"));
    }
}

void AXQPGameStateBase::ApplyMove2P(TWeakObjectPtr<AChesses> target, FChessMove2P move)
{
    HUD2P->AddOperatingRecord(battleTurn, target, move); // 记录走子
    SwitchBattleTurn(); // 轮换执棋
    board2P->ApplyMove(target, move);
}

void AXQPGameStateBase::OnFinishMove2P()
{
    if (bGameOver)
    {
        ULogger::Log(TEXT("AXQPGameStateBase::OnFinishMove2P: GameOver"));
        return;
    }

    switch (battleTurn) // 表示当前该谁了
    {
    case EPlayerTag::P1:
        HUD2P->SetAITurn(false); // 更新AI回合结束
        board2P->SetSideToMove(EChessColor::RED);
        break;
    case EPlayerTag::AI:
        board2P->SetSideToMove(EChessColor::BLACK);
        RunAI2P(); // 轮到AI
        break;
    case EPlayerTag::P2:
        board2P->SetSideToMove(EChessColor::BLACK);
        break;
    }
    UpdateScore(); // 更新得分
}

void AXQPGameStateBase::RunAI2P()
{
    HUD2P->SetAITurn(true);
    AI2P->SetBoard(board2P); // 棋盘状态

     AIAsync = UAsyncWorker::CreateAndStartWorker(
         [this](std::atomic<bool>& bShouldStop)
         {
             // 获取最佳移动方式和要移动的棋子 
             AIMovedChess = AI2P->GetBestMove(AIMove2P);
         },
         [this](EAsyncWorkerState State)
         {
             if (State != EAsyncWorkerState::Cancelled) // 任务正常执行完成
             {
                 // 应用棋子的移动
                 ApplyMove2P(AIMovedChess, AIMove2P);
                 ULogger::Log(TEXT("AXQPGameStateBase::RunAI2P: AI FINISH"));
             }
             else
             {
                 ULogger::LogWarning(TEXT("AXQPGameStateBase::RunAI2P: AI's work has been cancelled."));
             }
         }
    );
}

bool AXQPGameStateBase::IsMyTurn() const
{
    return MyPlayerTag == battleTurn;
}

void AXQPGameStateBase::SwitchBattleTurn()
{
    switch (battleType)
    {
    case EBattleType::P2:
        battleTurn = battleTurn == EPlayerTag::P1 ? EPlayerTag::P2 : EPlayerTag::P1;
        break;
    case EBattleType::P2_AI:
        battleTurn = battleTurn == EPlayerTag::AI ? EPlayerTag::P1 : EPlayerTag::AI;
        break;
    case EBattleType::P3:
        break;
    default:
        break;
    }
}

void AXQPGameStateBase::GameOver(EChessColor winner)
{
    bGameOver = true;
    HUD2P->ShowGameOver(winner);
}
