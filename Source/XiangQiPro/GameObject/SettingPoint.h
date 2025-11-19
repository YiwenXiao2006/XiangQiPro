// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/ChessMove.h"

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
class XIANGQIPRO_API ASettingPoint : public APawn
{
	GENERATED_BODY()

private:

	bool bActive = false;

	// 默认世界坐标
	FVector DefaultWorldPosition;

	// 简化坐标
	FVector2D Position2P;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 点击事件处理函数
	UFUNCTION()
	void OnComponentClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	// 鼠标悬停
	UFUNCTION()
	void OnComponentBeginCursorOver(UPrimitiveComponent* TouchedComponent);

	// 鼠标离开
	UFUNCTION()
	void OnComponentEndCursorOver(UPrimitiveComponent* TouchedComponent);

	// 激活组件
	void SetActivate(bool bInActive);

	// 设置目标棋子
	void SetTargetChess(TWeakObjectPtr<AChesses> targetChess);

	// 获取目标棋子
	TWeakObjectPtr<AChesses> GetTargetChess();

	FVector2D GetPosition2P();

	void SetPosition2P(FVector2D InPosition2P);

};
