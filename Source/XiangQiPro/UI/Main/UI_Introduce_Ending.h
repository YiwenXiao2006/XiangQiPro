// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_Introduce_Ending.generated.h"

class UListView; 
class UUI_Item_EndingGame;
class UEndingGameData;

/**
 * 残局游戏介绍界面
 */
UCLASS()
class XIANGQIPRO_API UUI_Introduce_Ending : public UUserWidget
{
	GENERATED_BODY()

	TArray<UUI_Item_EndingGame*> ListItems;

	TArray<UEndingGameData*> DataObjList;

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (BindWidget))
	UListView* EndingGameList;

protected:

	UPROPERTY(BlueprintReadOnly)
	int32 UserSelectedIndex = 0;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnListItemClicked(UUI_Item_EndingGame* ClickedItem);

	void SetSelection(int32 Index);

	void ClearSelection();
};
