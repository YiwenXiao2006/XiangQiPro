// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/MultiLineEditableTextBox.h"

#include "UI_Battle2P_Base.h"
#include "UI_TransitionScreen.h"
#include "UI_InGamePause.h"
#include "UIManager.h"

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "XQP_HUD.generated.h"

#define PATH_UI_INGAMEPAUSE TEXT("/Game/UMG/Pause/BP_UI_InGamePause.BP_UI_InGamePause_C")
#define PATH_UI_BATTLE2P_BASE TEXT("/Game/UMG/Battle2P/BP_UI_Battle2P_Base.BP_UI_Battle2P_Base_C")
#define PATH_UI_TRANSITIONSCREEN TEXT("/Game/UMG/Util/BP_UI_TransitionScreen.BP_UI_TransitionScreen_C")

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

	TSubclassOf<UI_TransitionScreen> Class_TransitionScreen;

	TSubclassOf<UI_InGamePause> Class_InGamePause;
	
};
