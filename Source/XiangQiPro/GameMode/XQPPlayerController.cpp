// Copyright 2026 Ultimate Player All Rights Reserved.


#include "XQPPlayerController.h"
#include "../UI/UIManager.h"

void AXQPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 显示鼠标、启用点击事件、启用滑动事件
	this->bEnableClickEvents = true;
	this->bEnableMouseOverEvents = true;
	this->bShowMouseCursor = true;
	this->bEnableTouchEvents = true;
	this->bEnableTouchOverEvents = true;
}

void AXQPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 绑定Esc按键
	InputComponent->BindAction("Escape", IE_Pressed, this, &AXQPPlayerController::OnEscapePressed).bExecuteWhenPaused = true;
}

void AXQPPlayerController::OnEscapePressed()
{
	if (UUIManager* UIManager = GetGameInstance()->GetSubsystem<UUIManager>())
	{
		UIManager->FinishUI();
	}
}
