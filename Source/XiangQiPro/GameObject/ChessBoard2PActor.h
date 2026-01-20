// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "XiangQiPro/Interface/IF_GameState.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessBoard2PActor.generated.h"

#define PATH_SM_CHESSBOARD_2P TEXT("/Script/Engine.StaticMesh'/Game/Mesh/ChessBoard/SM_ChessBoard_2P.SM_ChessBoard_2P'")

class UStaticMeshComponent;
class UChessBoard2P;

UCLASS()
class XIANGQIPRO_API AChessBoard2PActor : public AActor, public IIF_GameState
{
	GENERATED_BODY()

public:

	// 棋盘左下角坐标
	FVector BorderLoc1 = FVector(-38, -30, 1.3f);

	// 棋盘右上角坐标
	FVector BorderLoc2 = FVector(38, 30, 1.3f);
	
public:	
	// Sets default values for this actor's properties
	AChessBoard2PActor();

	UStaticMeshComponent* ChessBoardMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GamePlayAgain(UObject* OwnerObject) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 生成棋子Actor并保存到ChessBoard2P
	void GenerateChesses(TWeakObjectPtr<UChessBoard2P> board2P);

};
