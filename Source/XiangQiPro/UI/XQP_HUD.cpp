// Copyright 2026 Ultimate Player All Rights Reserved.

#include "XQP_HUD.h"
#include "../Util/ObjectManager.h"

AXQP_HUD::AXQP_HUD()
{
	Class_Battle2P_Base = OM::GetConstructorBlueprint<UI_Battle2P_Base>(PATH_UI_BATTLE2P_BASE);
	Class_TransitionScreen = OM::GetConstructorBlueprint<UI_TransitionScreen>(PATH_UI_TRANSITIONSCREEN);
	Class_InGamePause = OM::GetConstructorBlueprint<UI_InGamePause>(PATH_UI_INGAMEPAUSE);
}
