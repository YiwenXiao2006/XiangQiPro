// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI_Item_ClassicGame.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnListItemClicked, UUI_Item_ClassicGame*, ClickedItem);

/**
 * 经典游戏列表条目
 */
UCLASS()
class XIANGQIPRO_API UUI_Item_ClassicGame : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override; 

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UFUNCTION(BlueprintCallable)
    void SetSelected(bool bIsSelected);

    UFUNCTION(BlueprintCallable)
    void SetIndex(int32 NewIndex) { Index = NewIndex; }

    UFUNCTION(BlueprintCallable)
    int32 GetIndex() const { return Index; }

    // 点击事件
    UFUNCTION(BlueprintCallable)
    void OnItemClicked();

    UPROPERTY(BlueprintAssignable)
    FOnListItemClicked OnItemClickedDelegate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UButton* ItemButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* ItemText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UImage* SelectedIndicator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UImage* Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor DefaultColor = FLinearColor(0.f, 0.f, 0.f, 0.3f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor SelectedColor = FLinearColor(0.63f, 0.04f, 0.05f, 1.0f);

private:
    bool bIsInitItem = false;
    bool bSelected = false;
    int32 Index = -1;

    FLinearColor DefaultTextColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
    FLinearColor SelectedTextColor = FLinearColor(0.831373f, 0.686275f, 0.215686f, 1.0f);
};
