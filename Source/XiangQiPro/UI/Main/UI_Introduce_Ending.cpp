// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_Introduce_Ending.h"
#include "Components/ListView.h"
#include "EndingGameData.h"
#include "XiangQiPro/SaveGame/SaveGameLibrary.h"
#include "XiangQiPro/Util/EndingLibrary.h"

void UUI_Introduce_Ending::NativeConstruct()
{
    Super::NativeConstruct();

    EndingGameList->ClearListItems();

    int32 MaxIndex = USaveGameLibrary::GetEndingGameLevel_Max();
    UserSelectedIndex = USaveGameLibrary::GetEndingGameLevel();
    for (int32 i = 0; i <= MaxIndex; i++)
    {
        UEndingGameData* DataObj = NewObject<UEndingGameData>();
        if (DataObj)
        {
            FOnEndingGameListItemClicked CallBackFunc;
            CallBackFunc.AddDynamic(this, &UUI_Introduce_Ending::OnListItemClicked);

            DataObj->Init(i, &UserSelectedIndex, CallBackFunc);

            EndingGameList->AddItem(DataObj);
            DataObjList.Add(DataObj);
        }
    }
}

void UUI_Introduce_Ending::OnListItemClicked(UUI_Item_EndingGame* ClickedItem)
{
    if (!ClickedItem) return;

    ClearSelection();

    // 设置新的选择
    UserSelectedIndex = ClickedItem->Index;
    USaveGameLibrary::SetEndingGameLevel(UserSelectedIndex); // 更新要玩的关卡
    ClickedItem->SetSelected(true);
}

void UUI_Introduce_Ending::SetSelection(int32 Index)
{
    UUI_Item_EndingGame* Entry = EndingGameList->GetEntryWidgetFromItem<UUI_Item_EndingGame>(DataObjList[Index]);
    if (Entry)
    {
        Entry->SetSelected(true);
        OnListItemClicked(Entry);
    }
}

void UUI_Introduce_Ending::ClearSelection()
{
    ListItems.Empty();
    for (UEndingGameData*& Obj : DataObjList)
    {
        ListItems.Add(EndingGameList->GetEntryWidgetFromItem<UUI_Item_EndingGame>(Obj));
    }

    for (UUI_Item_EndingGame*& Item : ListItems)
    {
        if (Item)
        {
            Item->SetSelected(false);
        }
    }
    UserSelectedIndex = 0;
}

