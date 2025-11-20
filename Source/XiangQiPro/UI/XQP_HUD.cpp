// Copyright 2026 Ultimate Player All Rights Reserved.

#include "XQP_HUD.h"
#include "../Util/ObjectManager.h"

AXQP_HUD::AXQP_HUD()
{
	Class_Battle2P_Base = OM::GetConstructorBlueprint<UI_Battle2P_Base>(PATH_UI_BATTLE2P_BASE);
}
