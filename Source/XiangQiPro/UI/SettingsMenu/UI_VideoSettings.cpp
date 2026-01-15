// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_VideoSettings.h"
#include "XiangQiPro/UI/XQP_HUD.h"
#include <Kismet/KismetSystemLibrary.h>
#include <XiangQiPro/Util/Logger.h>

void UUI_VideoSettings::NativeConstruct()
{
	Super::NativeConstruct();
	if (UserSettings)
	{
		InitDisplayMode();
		InitResolution();
		InitVsync();
		InitResolutionRatio();
	}
}

void UUI_VideoSettings::InitDisplayMode()
{
	for (const FString& displayMode : DisplayModeList)
	{
		DisplayMode->AddOption(displayMode);
	}

	// 将下拉列表定位到当前显示模式
	DisplayMode->SetSelectedIndex(WindowMode.Find(UserSettings->GetFullscreenMode()));
}

void UUI_VideoSettings::InitResolution()
{
	Resolutions.Empty();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	for (const FIntPoint& resolution : Resolutions)
	{
		// 获取所有可用的分辨率 例如 1920 × 1080
		FString option = FString();
		option.Append(FString::FromInt(resolution.X));
		option.Append(UTF8_TO_TCHAR("×"));
		option.Append(FString::FromInt(resolution.Y));
		Resolution->AddOption(option);
	}

	// 将下拉列表定位到当前分辨率
	Resolution->SetSelectedIndex(Resolutions.Find(UserSettings->GetScreenResolution()));
}

void UUI_VideoSettings::InitVsync()
{
	for (const FString& vsyncMode : VsyncModeList)
	{
		VSyncMode->AddOption(vsyncMode);
	}
	VSyncMode->SetSelectedIndex(UserSettings->IsVSyncEnabled());
}

void UUI_VideoSettings::InitResolutionRatio()
{
	float CurrentScaleNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue;

	UserSettings->GetResolutionScaleInformationEx(CurrentScaleNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue);
	ResolutionRatioSlider->SetValue(CurrentScaleNormalized);

	int32 TextNum = CurrentScaleValue;
	ResolutionRatioText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), TextNum)));
}
