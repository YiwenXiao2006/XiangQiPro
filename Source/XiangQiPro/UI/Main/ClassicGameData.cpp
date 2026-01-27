// Copyright 2026 Ultimate Player All Rights Reserved.


#include "ClassicGameData.h"

void UClassicGameData::Init(int32 InIndex, FText InItemText, UTexture2D* InIcon, FOnClassicGameListItemClicked CallBackFunc)
{
	Index = InIndex;
	ItemText = InItemText;
	Icon = InIcon;
	OnItemClickedDelegate = CallBackFunc;
}
