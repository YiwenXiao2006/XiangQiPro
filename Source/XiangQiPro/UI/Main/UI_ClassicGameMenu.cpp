// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_ClassicGameMenu.h"
#include "UI_Item_ClassicGame.h"
#include "ClassicGameData.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

void UUI_ClassicGameMenu::NativeConstruct()
{
    Super::NativeConstruct();

}

void UUI_ClassicGameMenu::InitializeList(const TArray<FString>& Items, const TArray<UTexture2D*>& Icons)
{
    if (!ItemListView) return;

    ItemListView->ClearListItems();

    for (int32 i = 0; i < Items.Num(); i++)
    {
        UClassicGameData* DataObj = NewObject<UClassicGameData>();
        if (DataObj)
        {
            FOnListItemClicked CallBackFunc;
            CallBackFunc.AddDynamic(this, &UUI_ClassicGameMenu::OnListItemClicked);

            DataObj->Init(i, FText::FromString(Items[i]), Icons[i], CallBackFunc);

            ItemListView->AddItem(DataObj);
            DataObjList.Add(DataObj);
        }
    }
}

void UUI_ClassicGameMenu::OnListItemClicked(UUI_Item_ClassicGame* ClickedItem)
{
    if (!ClickedItem) return;

    ClearSelection();

    // 设置新的选择
    SelectedIndex = ClickedItem->GetIndex();
    ClickedItem->SetSelected(true);

    // 触发事件
    OnSelectedChange(SelectedIndex);
}

void UUI_ClassicGameMenu::SetSelection(int32 Index)
{
    UUI_Item_ClassicGame* Entry = ItemListView->GetEntryWidgetFromItem<UUI_Item_ClassicGame>(DataObjList[Index]);
    if (Entry)
    {
        Entry->SetSelected(true);
        OnListItemClicked(Entry);
    }
}

void UUI_ClassicGameMenu::ClearSelection()
{
    ListItems.Empty();
    for (UClassicGameData*& Obj : DataObjList)
    {
        ListItems.Add(ItemListView->GetEntryWidgetFromItem<UUI_Item_ClassicGame>(Obj));
    }

    for (UUI_Item_ClassicGame*& Item : ListItems)
    {
        if (Item)
        {
            Item->SetSelected(false);
        }
    }
    SelectedIndex = -1;
}

