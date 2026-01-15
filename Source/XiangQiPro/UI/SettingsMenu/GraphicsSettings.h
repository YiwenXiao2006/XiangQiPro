// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "GraphicsSettings.generated.h"

/**
 * 画面设置和视频设置的父类
 */
UCLASS()
class XIANGQIPRO_API UGraphicsSettings : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly)
	UGameUserSettings* UserSettings;
	
protected:

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SaveAndApply();
};
