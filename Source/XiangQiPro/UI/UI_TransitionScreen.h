// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/DoOnce.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_TransitionScreen.generated.h"

DECLARE_DELEGATE(FOnFadeFinished);
DECLARE_DYNAMIC_DELEGATE(FFadedFinishedDelegate);

typedef UUI_TransitionScreen UI_TransitionScreen;

class UUIManager;
class UImage;

/**
 * 转场用渐变UserWidget
 */
UCLASS()
class XIANGQIPRO_API UUI_TransitionScreen : public UUserWidget
{
	GENERATED_BODY()

private:

	FTimerHandle Handle_EndStay;

	FDoOnce<void()> PlayTransition;

	FDoOnce<void()> Reverse;

	UUIManager* UIManager;

	FVector2D FadeAlpha;

	bool bReverse = false;

	float FadeTime;

	float NewFadeTime;

	float StayTime;

	FLinearColor FadeColor;

	FOnFadeFinished Delegate = FOnFadeFinished();

	FFadedFinishedDelegate Delegate_D = FFadedFinishedDelegate();

public:

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Background;

protected:

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

public:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	* @brief 初始化转场屏幕
	* @param InUIManager 用户界面管理器
	* @param FromAlpha 初始透明度
	* @param ToAlpha 结束透明度
	* @param InFadeTime 淡化时间
	* @param InFadeColor 淡化颜色
	* @param InStayTime 淡化停留时间
	* @param bInReverse 倒放恢复
	* @return 两数之和
	*/
	void Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, float InStayTime = 0, bool bInReverse = false);

	/**
	* @brief 初始化转场屏幕
	* @param InUIManager 用户界面管理器
	* @param FromAlpha 初始透明度
	* @param ToAlpha 结束透明度
	* @param InFadeTime 淡化时间
	* @param InFadeColor 淡化颜色
	* @param InDelegate 淡化结束回调
	* @param InStayTime 淡化停留时间
	* @param bInReverse 倒放恢复
	*/
	void Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, FOnFadeFinished InDelegate, float InStayTime = 0, bool bInReverse = false);

	UFUNCTION(BlueprintCallable)
	void Init(UUIManager* InUIManager, float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, FFadedFinishedDelegate InDelegate, float InStayTime = 0, bool bInReverse = false);

	/* 结束淡化停留回调 */
	UFUNCTION()
	void EndStay();

	void EndFade();
	
};
