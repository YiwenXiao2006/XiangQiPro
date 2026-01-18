// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QualitySettingObject.generated.h"

/**
 * 画面设置列表数据
 */
UCLASS()
class XIANGQIPRO_API UQualitySettingObject : public UObject
{
	GENERATED_BODY()

public:

	TFunction<void(int32)> OnSelectionChangedCallBack;

	FText QualityName;

	int32 DefaultSelect = 0;

	void Init(int32 InDefaultSelect, FText InQualityName, TFunction<void(int32)> CallBackFunc);
	
};
