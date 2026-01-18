// Copyright 2026 Ultimate Player All Rights Reserved.


#include "ClassicGameData.h"

void UClassicGameData::Init(int32 InIndex, FText InItemText, FOnListItemClicked CallBackFunc)
{
	Index = InIndex;
	ItemText = InItemText;
	OnItemClickedDelegate = CallBackFunc;
}
