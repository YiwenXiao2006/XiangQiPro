// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_ClassicGameMenu.generated.h"

/**
 * 经典游戏菜单
 */
UCLASS()
class XIANGQIPRO_API UUI_ClassicGameMenu : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void InitializeList(const TArray<FString>& Items, const TArray<UTexture2D*>& Icons);

    UFUNCTION(BlueprintCallable)
    void SetSelection(int32 Index);

    UFUNCTION(BlueprintCallable)
    void ClearSelection();

    UFUNCTION(BlueprintImplementableEvent)
    void OnSelectedChange(int32 Index);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UListView* ItemListView;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class UUI_Item_ClassicGame> ListItemClass;

    UPROPERTY(BlueprintReadOnly)
    int32 SelectedIndex = -1;

private:
    UFUNCTION()
    void OnListItemClicked(class UUI_Item_ClassicGame* ClickedItem);

    TArray<class UUI_Item_ClassicGame*> ListItems;

    TArray<class UClassicGameData*> DataObjList;
	
};
