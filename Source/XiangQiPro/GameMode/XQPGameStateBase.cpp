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

AXQPGameStateBase::AXQPGameStateBase() : Super(), battleType(EBattleType::P2)
{
}

void AXQPGameStateBase::BeginPlay()
{
	Super::BeginPlay();
}

void AXQPGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

EBattleTurn AXQPGameStateBase::GetBattleTurn()
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

        AI2P->SetDepth(8); // 搜索深度
        AI2P->SetBoard(board2P); // 棋盘状态
    }
    else
    {
        ULogger::LogError(TEXT("AXQPGameStateBase::Start2PGame: Can't start two players game, because board2P actor is nullptr!"));
    }
}

void AXQPGameStateBase::ApplyMove2P(TWeakObjectPtr<AChesses> target, FChessMove2P move)
{
    board2P->ApplyMove(target, move);
    if (bGameOver)
    {
        ULogger::Log(TEXT("GameOver"));
        return;
    }
    UpdateScore(); // 更新得分

    RunAI2P(); // 轮到AI
}

void AXQPGameStateBase::RunAI2P()
{
    AI2P->SetBoard(board2P); // 棋盘状态
    battleTurn = EBattleTurn::AI;
    board2P->SetSideToMove(EChessColor::BLACK);
    HUD2P->SetAITurn(true);

    // 使用Async函数将任务提交到线程池
    Async(EAsyncExecution::ThreadPool, [this]()
        {
            // 获取最佳移动方式和要移动的棋子
            FChessMove2P aiMove;
            TWeakObjectPtr<AChesses> movedChess = AI2P->GetBestMove(aiMove);

            // 使用AsyncTask将后续操作派送回游戏线程
            AsyncTask(ENamedThreads::GameThread, [this, aiMove, movedChess]()
                {
                    // 应用棋子的移动
                    board2P->ApplyMove(movedChess, aiMove);
                    battleTurn = EBattleTurn::P1;
                    board2P->SetSideToMove(EChessColor::RED);
                    UpdateScore(); // 更新得分
                    HUD2P->SetAITurn(false); // 更新AI回合结束
                });
        });
}

void AXQPGameStateBase::GameOver(EChessColor winner)
{
    bGameOver = true;
    HUD2P->ShowGameOver(winner);
}
