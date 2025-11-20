// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/ChessMove.h"

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "XQPGameStateBase.generated.h"

class UAI2P;
class UChessBoard2P;
class UUI_Battle2P_Base;
class AChessBoard2PActor;
class AChesses;

enum class EChessColor : uint8;

typedef AXQPGameStateBase GS;

UENUM(BlueprintType)
enum class EBattleType : uint8
{
	P2 = 0 UMETA(DisplayName = "2Players"),
	P3 = 1 UMETA(DisplayName = "3Players"),  
};

enum class EBattleTurn : uint8
{
	AI = 0,
	P1 = 1,
	P2 = 2,
	P3 = 3
};

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AXQPGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

private:

	TWeakObjectPtr<UAI2P> AI2P;

	TWeakObjectPtr<UChessBoard2P> board2P;

	TWeakObjectPtr<AChessBoard2PActor> board2PActor;

	EBattleType battleType;

	EBattleTurn battleTurn = EBattleTurn::P1;

	TWeakObjectPtr<UUI_Battle2P_Base> HUD2P;

	// 玩家1的得分
	int32 score1 = 0;

	// 玩家2的得分
	int32 score2 = 0;

	// 玩家3的得分
	int32 score3 = 0;

	bool bGameOver = false;

private:

	// 更新得分
	void UpdateScore();

public:

	AXQPGameStateBase();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 显示置棋位置标记
	void ShowSettingPoint2P(TArray<FChessMove2P> Moves, TWeakObjectPtr<AChesses> Target);

	// 使所有置棋位置标记消失
	void DismissSettingPoint2P();

	// 获取双人棋盘(非Actor)
	TWeakObjectPtr<UChessBoard2P> GetChessBoard2P();

	// 获取对战人数类型
	EBattleType GetBattleType();

	// 获取执棋对象
	EBattleTurn GetBattleTurn();

	void SetHUD2P(TWeakObjectPtr<UUI_Battle2P_Base> hud2P);

	// 获取玩家1的得分
	int32 GetScore1() const;

	// 获取玩家2的得分
	int32 GetScore2() const;

	// 获取玩家3的得分
	int32 GetScore3() const;

	// 开始双人游戏
	void Start2PGame(TWeakObjectPtr<AChessBoard2PActor> InBoard2PActor);

	// 应用棋子移动
	void ApplyMove2P(TWeakObjectPtr<AChesses> target, FChessMove2P move);

	// 运行双人象棋AI
	UFUNCTION(BlueprintCallable, Category = "AI")
	void RunAI2P();

	// 游戏结束
	void GameOver(EChessColor winner);
	
};
