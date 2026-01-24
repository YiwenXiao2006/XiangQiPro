// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "XiangQiPro/Interface/IF_GameState.h"
#include "XiangQiPro/Util/ChessMove.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SettingPoint.generated.h"

#define PATH_NS_SETTINGPOINT TEXT("/Script/Niagara.NiagaraSystem'/Game/Niagara/NiagaraSystem/NS_SettingPoint.NS_SettingPoint'")

class AChesses;
class AXQPGameStateBase;

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;

UCLASS()
class XIANGQIPRO_API ASettingPoint : public APawn, public IIF_GameState
{
	GENERATED_BODY()

private:

	bool bActive = false;

	// 默认世界坐标
	FVector DefaultWorldPosition;

	// 简化坐标
	Position Position2P;

	AXQPGameStateBase* GameState;

	// Niagara系统
	UNiagaraSystem* NS_SettingPoint;

	// 目标移动的棋子
	TWeakObjectPtr<AChesses> TargetChess;
	
public:	
	// Sets default values for this actor's properties
	ASettingPoint();

	UPROPERTY(EditAnywhere)
	UNiagaraComponent* PointNiagara;

	UPROPERTY(EditAnywhere)
	USphereComponent* Sphere;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GamePlayAgain(UObject* OwnerObject) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnInputTouchEnter(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void OnInputTouchLeave(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void OnInputTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	// 统一的悬停进入处理
	UFUNCTION()
	void HandleHoverStart(UPrimitiveComponent* TouchedComponent);

	// 统一的悬停离开处理
	UFUNCTION()
	void HandleHoverEnd(UPrimitiveComponent* TouchedComponent);

	// 统一的点击处理
	UFUNCTION()
	void HandleClick(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	// 激活组件
	void SetActivate(bool bInActive);

	// 设置目标棋子
	void SetTargetChess(TWeakObjectPtr<AChesses> targetChess);

	// 获取目标棋子
	TWeakObjectPtr<AChesses> GetTargetChess();

	FVector2D GetPosition2P();

	void SetPosition2P(Position InPosition2P);

};
