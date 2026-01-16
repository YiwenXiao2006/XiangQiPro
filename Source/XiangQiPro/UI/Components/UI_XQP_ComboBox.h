// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "Components/ComboBoxString.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_XQP_ComboBox.generated.h"

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API UUI_XQP_ComboBox : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ComboBox;
	
};
