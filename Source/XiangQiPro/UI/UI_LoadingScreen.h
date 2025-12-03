// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_LoadingScreen.generated.h"

typedef UUI_LoadingScreen UI_LoadingScreen;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API UUI_LoadingScreen : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	virtual void NativeConstruct() override;

};
