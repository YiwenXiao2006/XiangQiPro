// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Chesses.h"

// Sets default values
AChesses::AChesses()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ChessMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChessMeshComponent"));
	ChessMesh->SetStaticMesh(OM::GetConstructorObject<UStaticMesh>(PATH_SM_CHESS));
	RootComponent = ChessMesh;

	MI_ChessMask = OM::GetConstructorObject<UMaterialInterface>(PATH_M_CHESSMASK);
	MI_Stroke = OM::GetConstructorObject<UMaterialInterface>(PATH_MI_STROKE_CHESS);

	ChessMask = CreateDefaultSubobject<UDecalComponent>(TEXT("Mask"));
	ChessMask->SetDecalMaterial(MI_ChessMask);
	ChessMask->SetupAttachment(ChessMesh);
	ChessMask->SetRelativeLocation(FVector(0, 0, 1));
	ChessMask->SetRelativeScale3D(FVector(0.0025f, 0.008f, 0.008f));
}

void AChesses::Init(EChessColor color, FVector2D pos, TWeakObjectPtr<UChessBoard2P> board2P)
{
	MyColor = color;
	Pos = pos;
	Board2P = board2P;

	GameState = Cast<GS>(GetWorld()->GetGameState());
	if (GameState)
	{
		if (GameState->GetBattleType() == EBattleType::P2)
		{
			if (color == EChessColor::RED)
			{
				ChessMask->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, -90, 90)));
			}
			else if (color == EChessColor::BLACK)
			{
				ChessMask->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, -90, -90)));
			}
		}
	}
}

// Called when the game starts or when spawned
void AChesses::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AChesses::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AChesses::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AChesses::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);

	if (ButtonPressed == EKeys::LeftMouseButton)
	{
		if (GameState)
		{
			if (GameState->GetBattleTurn() != EBattleTurn::AI)
			{
				if (GameState->GetBattleType() == EBattleType::P2)
				{
					if (MyColor != EChessColor::BLACK) // 不是AI的棋子
					{
						if (Board2P.IsValid())
						{
							GenerateMove2P(Board2P);
						}
						else
						{
							ULogger::LogError(TEXT("Chesses OnClicked: ChessBoard2P instance is nullptr!"));
						}
					}
				}
			}
		}
		else
		{
			ULogger::LogError(TEXT("Chesses OnClicked: GameState instance is nullptr!"));
		}
	}
}

void AChesses::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	ChessMesh->SetOverlayMaterial(MI_Stroke); // 添加描边材质
}

void AChesses::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	ChessMesh->SetOverlayMaterial(nullptr); // 移除描边材质
}

void AChesses::Defeated()
{
	SetActorHiddenInGame(true); // 吃掉，将其隐藏
	ChessMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 关闭碰撞体积
}

EChessColor AChesses::GetColor() const
{
	return MyColor;
}

EChessType AChesses::GetType() const
{
	return MyType;
}

FVector2D AChesses::GetSimpPosition() const
{
	return Pos;
}

void AChesses::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P)
{
}

void AChesses::ApplyMove(FChessMove2P Move)
{
	Pos = FVector2D(Move.to.X, Move.to.Y); // 更新简化坐标

	// 更新世界坐标
	FVector WorldLoc = Board2P->BoardLocs[Move.to.X][Move.to.Y];
	SetActorLocation(WorldLoc);

	GameState->DismissSettingPoint2P(); // 隐藏所有落子点
}

void AChesses::PlayMoveAnim()
{
}

