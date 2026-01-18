// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UI_Item_ClassicGame.h"
#include "ClassicGameData.generated.h"

/**
 * 经典游戏条目数据
 */
UCLASS()
class XIANGQIPRO_API UClassicGameData : public UObject
{
	GENERATED_BODY()

public:

	FOnListItemClicked OnItemClickedDelegate;

	int32 Index = 0;

	FText ItemText;

	void Init(int32 InIndex, FText InItemText, FOnListItemClicked CallBackFunc);
	
};
