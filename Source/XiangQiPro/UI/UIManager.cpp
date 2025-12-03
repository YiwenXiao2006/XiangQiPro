// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UIManager.h"

void UUIManager::Init(UUserWidget* InBasicUI)
{
	ui_stack.Empty();
	BasicUI = InBasicUI;
	if (BasicUI)
		BasicUI->AddToPlayerScreen();
}

void UUIManager::Init(UUserWidget* InBasicUI, UUserWidget* InPauseUI)
{
	ui_stack.Empty();
	BasicUI = InBasicUI;
	PauseUI = InPauseUI;
	if (BasicUI)
		BasicUI->AddToPlayerScreen();
}

void UUIManager::AddUI(UUserWidget* InUI)
{
	if (!InUI)
	{
		return; // 空指针保护
	}

	if (!ui_stack.IsEmpty())
	{
		SetUIVisibility(ui_stack.Top(), false); // 隐藏最上层UI
	}
	else
	{
		SetUIVisibility(BasicUI, false);		// 隐藏基本UI
	}
	InUI->AddToPlayerScreen(); // 添加UI到玩家屏幕
	ui_stack.Add(InUI);		   // 把UI加入栈
}

void UUIManager::FinishUI()
{
	if (ui_stack.IsEmpty())
	{
		if (PauseUI) // 仅当暂停界面存在时才允许隐藏基础UI
		{
			SetUIVisibility(BasicUI, false); // 隐藏基础UI
			AddUI(PauseUI); // 添加暂停界面
		}
	}
	else
	{
		if (UUserWidget* pop_ui = ui_stack.Pop())
		{
			pop_ui->RemoveFromViewport(); // 移除顶层UI
		}

		if (ui_stack.IsEmpty())
		{
			SetUIVisibility(BasicUI, true); // 重新展示基础UI
		}
		else
		{
			SetUIVisibility(ui_stack.Top(), true); // 重新显示上一层UI
		}
	}
}

void UUIManager::SetUIVisibility(UUserWidget* ui, bool bVisible)
{
	if (ui)
	{
		ui->SetIsEnabled(bVisible);
		ui->SetColorAndOpacity(bVisible ? FLinearColor(1, 1, 1, 1) : FLinearColor(0, 0, 0, 0));
	}
}
