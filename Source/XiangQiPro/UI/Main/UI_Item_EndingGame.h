// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI_Item_EndingGame.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndingGameListItemClicked, UUI_Item_EndingGame*, ClickedItem);

/**
 * 残局关卡条目
 */
UCLASS()
class XIANGQIPRO_API UUI_Item_EndingGame : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

	bool bIsInitItem = false;
	bool bSelected = false;
	int32* UserSelectedIndex = nullptr;

	FLinearColor DefaultTextColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FLinearColor SelectedTextColor = FLinearColor(0.831373f, 0.686275f, 0.215686f, 1.0f);
	FLinearColor DefaultColor = FLinearColor(0.f, 0.f, 0.f, 0.3f);
	FLinearColor SelectedColor = FLinearColor(0.63f, 0.04f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* IndexText;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UButton* ItemButton;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UImage* SelectedIndicator;

protected:

	UPROPERTY(BlueprintAssignable)
	FOnEndingGameListItemClicked OnItemClickedDelegate;

	virtual void NativeConstruct() override;

	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	virtual void NativeOnEntryReleased() override;

	UFUNCTION()
	void OnItemClicked();

public:

	int32 Index = 0;

	void SetSelected(bool bIsSelected);

};
