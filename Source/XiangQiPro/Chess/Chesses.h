// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/ObjectManager.h"
#include "../Util/ChessInfo.h"
#include "../Util/ChessMove.h"

#include "../GameObject/ChessBoard2P.h"

#include "../GameMode/XQPGameStateBase.h"

#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Chesses.generated.h"

#define PATH_SM_CHESS TEXT("/Script/Engine.StaticMesh'/Game/Mesh/Chess/SM_Chess.SM_Chess'")
#define PATH_M_CHESSMASK TEXT("/Script/Engine.Material'/Game/Material/Decal/ChessMask/M_ChessMask.M_ChessMask'")
#define PATH_MI_STROKE_CHESS TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Material/Chess/MI_Stroke_Chess.MI_Stroke_Chess'")

struct FChessMove2P;

UCLASS()
class XIANGQIPRO_API AChesses : public APawn
{
	GENERATED_BODY()

protected:

	// 棋子颜色(阵营)
	EChessColor MyColor;

	// 棋子类型
	EChessType MyType;

	// 棋子当前的简化坐标
	FVector2D Pos;

	AXQPGameStateBase* GameState;

	TWeakObjectPtr<UChessBoard2P> Board2P;

	bool bSelected = false;

public:
	// Sets default values for this pawn's properties
	AChesses();

	virtual void Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P);
	 
	// 棋子静态网格体组件
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ChessMesh;

	// 棋子上文字的贴花组件
	UPROPERTY(EditAnywhere)
	UDecalComponent* ChessMask;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_ChessMask;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_Stroke;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// 点击事件处理函数
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;

	virtual void NotifyActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex) override;

	// 鼠标悬停
	virtual void NotifyActorBeginCursorOver() override;

	// 鼠标离开
	virtual void NotifyActorEndCursorOver() override;

	virtual void NotifyActorOnInputTouchBegin(const ETouchIndex::Type FingerIndex) override;

	// 处理点击事件
	void HandleClick();

	// 棋子战败
	virtual void Defeated();

	UFUNCTION(BlueprintPure, Category = "Chess")
	EChessColor GetColor() const;

	UFUNCTION(BlueprintPure, Category = "Chess")
	EChessType GetType() const;

	FVector2D GetSimpPosition() const;

	virtual void GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target);

	virtual void ApplyMove(FChessMove2P Move);

	virtual void PlayMoveAnim();
};
