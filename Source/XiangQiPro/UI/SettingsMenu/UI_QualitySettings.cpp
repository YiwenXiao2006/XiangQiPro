// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_QualitySettings.h"
#include "XiangQiPro/UI/XQP_HUD.h"

void UUI_QualitySettings::NativeConstruct()
{
	Super::NativeConstruct();

	ADD_QUALITY_OPTION("后期效果", PostProcessing);
	ADD_QUALITY_OPTION("抗锯齿", AntiAliasing);
	ADD_QUALITY_OPTION("阴影质量", Shadow);
	ADD_QUALITY_OPTION("反射质量", Reflection);
	ADD_QUALITY_OPTION("全局光照质量", GlobalIllumination);
	ADD_QUALITY_OPTION("贴图质量", Texture);
	ADD_QUALITY_OPTION("细节质量", Foliage);
	ADD_QUALITY_OPTION("特效质量", VisualEffect);
	ADD_QUALITY_OPTION("着色质量", Shading);
}
