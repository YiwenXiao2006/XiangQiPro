// Copyright 2026 Ultimate Player All Rights Reserved.


#include "QualitySettingObject.h"

void UQualitySettingObject::Init(int32 InDefaultSelect, FText InQualityName, TFunction<void(int32)> CallBackFunc)
{
	DefaultSelect = InDefaultSelect;
	QualityName = InQualityName;
	OnSelectionChangedCallBack = CallBackFunc;
}
