// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "UI_Battle2P_Base.h"

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "XQP_HUD.generated.h"

#define PATH_UI_BATTLE2P_BASE TEXT("/Game/UMG/Battle2P/UI_Battle2P_Base.UI_Battle2P_Base_C")

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AXQP_HUD : public AHUD
{
	GENERATED_BODY()
	
public:

	AXQP_HUD();


	TSubclassOf<UI_Battle2P_Base> Class_Battle2P_Base;
	
};
