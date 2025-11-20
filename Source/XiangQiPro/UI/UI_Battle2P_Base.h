// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_Battle2P_Base.generated.h"

class UTextBlock;

typedef UUI_Battle2P_Base UI_Battle2P_Base;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API UUI_Battle2P_Base : public UUserWidget
{
	GENERATED_BODY()

public:

	// 显示玩家1的名字
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* Text_Name_P1;

	// 显示玩家2的名字
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* Text_Name_P2;

	// 显示玩家2的名字
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* Text_Score_P1;

	// 显示玩家2的名字
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* Text_Score_P2;

public:

	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void UpdateScore(int32 score1, int32 score2);
	
};
