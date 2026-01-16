// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UI_Item_QualitySetting.h"

void UUI_Item_QualitySetting::NativeConstruct()
{
	Super::NativeConstruct(); 
	for (const FString& level : LevelText)
	{
		XQP_ComboBox->ComboBox->AddOption(level);
	}

	XQP_ComboBox->ComboBox->OnSelectionChanged.Clear();
	XQP_ComboBox->ComboBox->OnSelectionChanged.AddDynamic(this, &UUI_Item_QualitySetting::OnSelectionChanged);
}

void UUI_Item_QualitySetting::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (bIsInitItem)
	{
		return;
	}
	if (UQualitySettingObject* QSObj = Cast<UQualitySettingObject>(ListItemObject))
	{
		XQP_ComboBox->ComboBox->SetSelectedIndex(QSObj->DefaultSelect); // 已选择的质量
		QualityName = QSObj->QualityName;								// 画质选项名称
		OnSelectionChangedCallBack = QSObj->OnSelectionChangedCallBack; // 选择回调
		QualityText->SetText(QualityName);
		bIsInitItem = true;
	}
}

void UUI_Item_QualitySetting::OnSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (OnSelectionChangedCallBack)
		OnSelectionChangedCallBack(XQP_ComboBox->ComboBox->GetSelectedIndex()); // 选择后的回调函数
}
