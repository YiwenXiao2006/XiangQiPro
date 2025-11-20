// Copyright 2026 Ultimate Player All Rights Reserved.


#include "Camera2PActor.h"
#include "../GameMode/XQPGameStateBase.h"
#include "../UI/XQP_HUD.h"
#include "../Util/Logger.h"

ACamera2PActor::ACamera2PActor()
{
	// 创建根组件
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	// 创建弹簧臂组件
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 500.0f; // 摄像机距离
	SpringArm->bUsePawnControlRotation = false; // 重要：不使用Pawn控制旋转
	SpringArm->bEnableCameraLag = true; // 启用摄像机延迟
	SpringArm->CameraLagSpeed = 3.0f; // 延迟速度
	SpringArm->bDoCollisionTest = false; // 禁用碰撞检测（可选）

	// 设置弹簧臂旋转限制
	SpringArm->SetRelativeRotation(FRotator(-40.0f, 0.0f, 0.0f));

	// 创建摄像机组件
	GameCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	GameCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// 创建音频组件
	BattleAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("BattleAudio"));
	BattleAudio->SetupAttachment(GameCamera);

	// 初始化变量
	bIsRotatingCamera = false;
	TargetRotation = FRotator::ZeroRotator;

	// 自动控制玩家
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ACamera2PActor::BeginPlay()
{
	Super::BeginPlay();

	BattleAudio->SetSound(BattleMusic); // 设置音频资源
	BattleAudio->Play(); // 开始播放背景音乐

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		HUD = Cast<AXQP_HUD>(PC->GetHUD());
		if (HUD)
		{
			UI_Battle2P_Base* Base = CreateWidget<UI_Battle2P_Base>(GetWorld(), HUD->Class_Battle2P_Base);
			Base->AddToPlayerScreen();

			if (AXQPGameStateBase* GameState = Cast<GS>(GetWorld()->GetGameState()))
			{
				GameState->SetHUD2P(Base);
			}
			else
			{
				ULogger::LogError(TEXT("ACamera2PActor::BeginPlay: GameState is nullptr!"));
			}
		}
		else
		{
			ULogger::LogError(TEXT("ACamera2PActor::BeginPlay: HUD is nullptr!"));
		}
	}
	else
	{
		ULogger::LogError(TEXT("ACamera2PActor::BeginPlay: Player controller is nullptr!"));
	}

	// 初始化目标旋转为当前旋转
	TargetRotation = SpringArm->GetRelativeRotation();
}

// 如果需要每帧更新，可以重写Tick函数
void ACamera2PActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 平滑应用旋转
	if (!TargetRotation.Equals(SpringArm->GetRelativeRotation()))
	{
		FRotator CurrentRotation = SpringArm->GetRelativeRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 8.0f);
		SpringArm->SetRelativeRotation(NewRotation);
	}
}

void ACamera2PActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 绑定鼠标按键事件
	PlayerInputComponent->BindAction("CameraRotate", IE_Pressed, this, &ACamera2PActor::OnMouseRightButtonPressed);
	PlayerInputComponent->BindAction("CameraRotate", IE_Released, this, &ACamera2PActor::OnMouseRightButtonReleased);

	// 绑定鼠标轴事件
	PlayerInputComponent->BindAxis("MouseX", this, &ACamera2PActor::OnMouseX);
	PlayerInputComponent->BindAxis("MouseY", this, &ACamera2PActor::OnMouseY);

	// 绑定触摸输入
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACamera2PActor::OnTouchBegin);
	PlayerInputComponent->BindTouch(IE_Repeat, this, &ACamera2PActor::OnTouchMove);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACamera2PActor::OnTouchEnd);
}

void ACamera2PActor::OnMouseRightButtonPressed()
{
	bIsRotatingCamera = true;

	// 显示鼠标光标并锁定到视口
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;
	}
}

void ACamera2PActor::OnMouseRightButtonReleased()
{
	bIsRotatingCamera = false;

	// 恢复鼠标光标
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
}

void ACamera2PActor::OnMouseX(float Value)
{
	if (bIsRotatingCamera && FMath::Abs(Value) > 0.0f)
	{
		// 绕Z轴旋转（Yaw）
		TargetRotation.Yaw += Value * RotationSpeed;
	}
}

void ACamera2PActor::OnMouseY(float Value)
{
	if (bIsRotatingCamera && FMath::Abs(Value) > 0.0f)
	{
		// 绕X轴旋转（Pitch），并限制角度
		float NewPitch = TargetRotation.Pitch + Value * RotationSpeed;
		TargetRotation.Pitch = FMath::Clamp(NewPitch, MinPitchAngle, MaxPitchAngle);
	}
}

void ACamera2PActor::OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location)
{
	bool bIsCurrentlyPressed;
	if (FingerIndex == ETouchIndex::Touch1)
	{
		// 单指触摸开始
		FVector2D ScreenLocation;
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(FingerIndex, ScreenLocation.X, ScreenLocation.Y, bIsCurrentlyPressed);

		InitialTouchLocation = ScreenLocation;
		PreviousTouchLocation = ScreenLocation;
		bIsSingleTouchActive = true;
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		// 双指触摸开始
		FVector2D FirstFinger, SecondFinger;
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch1, FirstFinger.X, FirstFinger.Y, bIsCurrentlyPressed);
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch2, SecondFinger.X, SecondFinger.Y, bIsCurrentlyPressed);

		FirstFingerLocation = FirstFinger;
		SecondFingerLocation = SecondFinger;
		InitialFingerDistance = FVector2D::Distance(FirstFinger, SecondFinger);
		bIsTwoFingerTouchActive = true;
		bIsSingleTouchActive = false; // 双指触摸时禁用单指操作
	}
}

void ACamera2PActor::OnTouchMove(ETouchIndex::Type FingerIndex, FVector Location)
{
	bool bIsCurrentlyPressed;
	if (bIsTwoFingerTouchActive)
	{
		// 处理双指手势（缩放）
		HandleTwoFingerGesture();
	}
	else if (bIsSingleTouchActive && FingerIndex == ETouchIndex::Touch1)
	{
		// 处理单指滑动（旋转）
		FVector2D CurrentTouchLocation;
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(FingerIndex, CurrentTouchLocation.X, CurrentTouchLocation.Y, bIsCurrentlyPressed);

		// 计算滑动增量
		FVector2D Delta = CurrentTouchLocation - PreviousTouchLocation;

		// 应用旋转（Y轴反向，因为屏幕坐标Y轴向下）
		float NewYaw = TargetRotation.Yaw + Delta.X * RotationSensitivity * 0.1f;
		float NewPitch = TargetRotation.Pitch - Delta.Y * RotationSensitivity * 0.1f;

		// 限制俯仰角
		NewPitch = FMath::Clamp(NewPitch, MinPitchAngle, MaxPitchAngle);

		TargetRotation = FRotator(NewPitch, NewYaw, 0.0f);

		PreviousTouchLocation = CurrentTouchLocation;
	}
}

void ACamera2PActor::OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location)
{
	bool bIsCurrentlyPressed;
	if (FingerIndex == ETouchIndex::Touch1)
	{
		bIsSingleTouchActive = false;

		// 如果还有第二指触摸，切换到双指模式
		float DummyX, DummyY;
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch2, DummyX, DummyY, bIsCurrentlyPressed);
		if (bIsCurrentlyPressed)
		{
			bIsTwoFingerTouchActive = true;
		}
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		bIsTwoFingerTouchActive = false;

		// 如果还有第一指触摸，切换到单指模式
		float DummyX, DummyY;
		GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch1, DummyX, DummyY, bIsCurrentlyPressed);
		if (bIsCurrentlyPressed)
		{
			bIsSingleTouchActive = true;

			// 更新触摸位置
			GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch1, PreviousTouchLocation.X, PreviousTouchLocation.Y, bIsCurrentlyPressed);
		}
	}
}

void ACamera2PActor::HandleTwoFingerGesture()
{
	bool bIsCurrentlyPressed;
	FVector2D FirstFinger, SecondFinger;
	GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch1, FirstFinger.X, FirstFinger.Y, bIsCurrentlyPressed);
	GetWorld()->GetFirstPlayerController()->GetInputTouchState(ETouchIndex::Touch2, SecondFinger.X, SecondFinger.Y, bIsCurrentlyPressed);

	// 计算当前两指距离
	float CurrentDistance = FVector2D::Distance(FirstFinger, SecondFinger);

	// 计算缩放比例
	float ZoomDelta = (CurrentDistance - InitialFingerDistance) * ZoomSensitivity * 0.01f;

	// 应用缩放
	TargetArmLength = FMath::Clamp(TargetArmLength - ZoomDelta, MinZoomDistance, MaxZoomDistance);

	// 更新初始距离为当前距离，实现连续缩放
	InitialFingerDistance = CurrentDistance;
}