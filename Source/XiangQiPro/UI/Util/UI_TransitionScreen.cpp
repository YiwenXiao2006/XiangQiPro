// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_TransitionScreen.h"

#include "XiangQiPro/UI/XQP_HUD.h"

void UUI_TransitionScreen::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateCanTick();

	PlayTransition = FDoOnce<void()>([this]() {
		if (StayTime != 0)
		{
			FTimerDelegate Delegate_EndStay;
			Delegate_EndStay.BindUObject(this, &UUI_TransitionScreen::EndStay);
			GetWorld()->GetTimerManager().SetTimer(Handle_EndStay, Delegate_EndStay, StayTime, false);
			return;
		}

		EndFade(); 
		});

	Reverse = FDoOnce<void()>([this](){
		NewFadeTime = 0;
		FadeAlpha = FVector2D(FadeAlpha.Y, FadeAlpha.X);
		PlayTransition.SetExecutable(true); // 重新启用
		});

	Image_Background->SetColorAndOpacity(FLinearColor(FadeColor.R, FadeColor.G, FadeColor.B, FadeAlpha.X));
}

void UUI_TransitionScreen::NativeDestruct()
{
	// 清理资源
	if (IsValid(UIManager))
	{
		GetWorld()->GetTimerManager().ClearTimer(Handle_EndStay);
	}

	PlayTransition.Reset();
	Reverse.Reset();
	Super::NativeDestruct();
}

void UUI_TransitionScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	NewFadeTime += InDeltaTime;
	if (NewFadeTime > FadeTime)
	{
		NewFadeTime = FadeTime;

		// 屏幕转场完毕，执行回调函数
		if (PlayTransition.IsExecutable())
		{
			PlayTransition.Execute();
		}
	}

	OnFadingDelegate.ExecuteIfBound(NewFadeTime / FadeTime); // 正在淡化回调，传入淡化进度百分比

	Image_Background->SetColorAndOpacity(FLinearColor(FadeColor.R, FadeColor.G, FadeColor.B,
		(FadeAlpha.Y - FadeAlpha.X) * (NewFadeTime / FadeTime) + FadeAlpha.X));
}

void UUI_TransitionScreen::Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, float InStayTime, bool bInReverse)
{
	UIManager = InUIManager;
	FadeAlpha = FVector2D(FromAlpha, ToAlpha);
	FadeTime = InFadeTime;
	NewFadeTime = 0.f;
	FadeColor = InFadeColor;
	StayTime = InStayTime;
	bReverse = bInReverse;
}

void UUI_TransitionScreen::Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, FOnFadeFinished InDelegate, float InStayTime, bool bInReverse)
{
	UIManager = InUIManager;
	FadeAlpha = FVector2D(FromAlpha, ToAlpha);
	FadeTime = InFadeTime;
	NewFadeTime = 0.f;
	FadeColor = InFadeColor;
	Delegate = InDelegate;
	StayTime = InStayTime;
	bReverse = bInReverse;
}

UUI_TransitionScreen* UUI_TransitionScreen::Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, FOnFadeFinishedDynamic InDelegate, float InStayTime, bool bInReverse)
{
	UIManager = InUIManager;
	FadeAlpha = FVector2D(FromAlpha, ToAlpha);
	FadeTime = InFadeTime;
	NewFadeTime = 0.f;
	FadeColor = InFadeColor;
	Delegate_DYNAMIC = InDelegate;
	StayTime = InStayTime;
	bReverse = bInReverse;
	return this;
}

void UUI_TransitionScreen::SetOnFadingDelegate(FOnFadingDelegate InDelegate)
{
	OnFadingDelegate = InDelegate;
}

void UUI_TransitionScreen::EndStay()
{
	StayTime = 0; // 清除停留时间
	EndFade();    // 执行淡化结束回调
}

void UUI_TransitionScreen::EndFade()
{
	if (bReverse && Reverse.IsExecutable())
	{
		Reverse.Execute(); // 执行倒放操作
		return;
	}
	Delegate.ExecuteIfBound();
	Delegate_DYNAMIC.ExecuteIfBound();

	if (IsValid(UIManager))
	{
		UIManager->RemoveUI(this); // 移除过渡界面
	}
}

