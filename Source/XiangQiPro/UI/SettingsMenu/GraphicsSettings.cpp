// Copyright 2026 Ultimate Player All Rights Reserved.


#include "GraphicsSettings.h"

void UGraphicsSettings::NativeConstruct()
{
	Super::NativeConstruct();
	if (GEngine)
	{
		UserSettings = GEngine->GetGameUserSettings();
	}
}

void UGraphicsSettings::SaveAndApply()
{
	UserSettings->SaveSettings();
	UserSettings->ApplySettings(true);
}
