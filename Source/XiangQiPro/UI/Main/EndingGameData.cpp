// Copyright 2026 Ultimate Player All Rights Reserved.


#include "EndingGameData.h"

void UEndingGameData::Init(int32 InIndex, int32* InUserSelectedIndex, FOnEndingGameListItemClicked CallBackFunc)
{
	Index = InIndex;
	UserSelectedIndex = InUserSelectedIndex;
	OnItemClickedDelegate = CallBackFunc;
}
