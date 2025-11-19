// Copyright 2026 Ultimate Player All Rights Reserved.


#include "XQPPlayerController.h"

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
