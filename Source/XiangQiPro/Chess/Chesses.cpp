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
		HandleClick();
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
	if (!bSelected) // 未被选中
	{
		ChessMesh->SetOverlayMaterial(nullptr); // 移除描边材质
	}
}

void AChesses::NotifyActorOnInputTouchBegin(const ETouchIndex::Type FingerIndex)
{
	Super::NotifyActorOnInputTouchBegin(FingerIndex);
	if (FingerIndex == ETouchIndex::Touch1)
		if (!bSelected)
			ChessMesh->SetOverlayMaterial(MI_Stroke); // 添加描边材质
}

void AChesses::NotifyActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex)
{
	Super::NotifyActorOnInputTouchEnd(FingerIndex);

	// 对于触摸事件，我们直接处理，不区分手指索引（或者只处理第一根手指）
	if (FingerIndex == ETouchIndex::Touch1)
	{
		HandleClick();
	}
}

void AChesses::HandleClick()
{
	if (GameState)
	{
		if (GameState->GetBattleTurn() != EBattleTurn::AI)
		{
			if (GameState->GetBattleType() == EBattleType::P2)
			{
				if (MyColor != EChessColor::BLACK) // AI
				{
					if (bSelected)
					{
						bSelected = false; // 移除被选中状态
						GameState->DismissSettingPoint2P(); // 清除落子点
						ChessMesh->SetOverlayMaterial(nullptr); // 移除描边材质
					}
					else
					{
						if (Board2P.IsValid())
						{
							bSelected = true;
							ChessMesh->SetOverlayMaterial(MI_Stroke); // 添加描边材质
							GenerateMove2P(Board2P, this);
						}
						else
						{
							ULogger::LogError(TEXT("Chesses OnClicked: ChessBoard2P instance is nullptr!"));
						}
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

void AChesses::GenerateMove2P(TWeakObjectPtr<UChessBoard2P> board2P, TWeakObjectPtr<AChesses> target)
{
	for (int32 i = 0; i < 10; i++)
	{
		for (int32 j = 0; j < 9; j++)
		{
			TWeakObjectPtr<AChesses> captureChess = Board2P->AllChess[i][j];
			if (captureChess != nullptr && captureChess != target.Get())
			{
				captureChess->ChessMesh->SetOverlayMaterial(nullptr); // 移除描边材质
				captureChess->bSelected = false; // 清除被选择状态
			}
		}
	}
}

void AChesses::ApplyMove(FChessMove2P Move)
{
	Pos = FVector2D(Move.to.X, Move.to.Y); // 更新简化坐标

	// 更新世界坐标
	FVector WorldLoc = Board2P->BoardLocs[Move.to.X][Move.to.Y];
	SetActorLocation(WorldLoc);

	bSelected = false; // 移除被选中状态
	ChessMesh->SetOverlayMaterial(nullptr); // 移除描边材质
	GameState->DismissSettingPoint2P(); // 隐藏所有落子点
}

void AChesses::PlayMoveAnim()
{
}

