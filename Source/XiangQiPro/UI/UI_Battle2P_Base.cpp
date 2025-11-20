// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_Battle2P_Base.h"
#include "XQP_HUD.h"

void UUI_Battle2P_Base::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateCanTick();
}

void UUI_Battle2P_Base::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UUI_Battle2P_Base::UpdateScore(int32 score1, int32 score2)
{
	Text_Score_P1->SetText(FText::FromString(FString::FromInt(score1)));
	Text_Score_P2->SetText(FText::FromString(FString::FromInt(score2)));
}
