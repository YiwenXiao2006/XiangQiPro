// Copyright 2026 Ultimate Player All Rights Reserved.


#include "UIManager.h"
#include <XiangQiPro/Util/Logger.h>
#include <Kismet/GameplayStatics.h>
#include <Blueprint/WidgetBlueprintLibrary.h>

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
}

void UUIManager::Init(UUserWidget* InBasicUI, TSubclassOf<UUserWidget> InPauseWidgetClass)
{
	ui_stack.Empty();

	BasicUI = InBasicUI;
	PauseWidgetClass = InPauseWidgetClass;
	PauseUI.Reset();

	if (BasicUI)
		BasicUI->AddToPlayerScreen();
}

void UUIManager::AddUI(UUserWidget* InUI, bool bHideLastOne)
{
	if (!InUI)
	{
		return; // 空指针保护
	}

	if (bHideLastOne)
	{
		if (!ui_stack.IsEmpty())
		{
			SetUIVisibility(ui_stack.Top(), false); // 隐藏最上层UI
		}
		else
		{
			SetUIVisibility(BasicUI, false);		// 隐藏基本UI
		}
	}
	InUI->AddToPlayerScreen(); // 添加UI到玩家屏幕
	ui_stack.Add(InUI);		   // 把UI加入栈
}

void UUIManager::RemoveUI(UUserWidget* TargetUI)
{
	if (!TargetUI)
	{
		return; // 空指针保护
	}

	int32 Index; // 目标在UI栈中的索引
	if (!ui_stack.Find(TargetUI, Index)) // 如果目标不在栈中
	{
		return;
	}

	ui_stack[Index]->RemoveFromParent(); // 从屏幕中移除
	ui_stack.RemoveAt(Index); // 从栈中移除
	
	if (ui_stack.IsEmpty())
	{
		SetUIVisibility(BasicUI, true); // 重新展示基础UI
	}
	else
	{
		SetUIVisibility(ui_stack.Top(), true); // 重新显示上一层UI
	}
}

void UUIManager::FinishUI()
{
	if (ui_stack.IsEmpty())
	{
		if (!PauseUI.IsValid() && IsValid(PauseWidgetClass))
		{
			PauseUI = CreateWidget<UUserWidget>(GetWorld(), PauseWidgetClass);
		}

		if (PauseUI.IsValid()) // 仅当暂停界面存在时才允许隐藏基础UI
		{
			SetUIVisibility(BasicUI, false); // 隐藏基础UI
			AddUI(PauseUI.Get()); // 添加暂停界面
			ULogger::Log(TEXT("OnGamePause"));
			EXEC_GAMEPAUSE(); // 调用暂停事件
		}
	}
	else
	{
		if (UUserWidget* pop_ui = ui_stack.Pop())
		{
			if (pop_ui == PauseUI)
			{
				EXEC_GAMERESUME(); // 调用恢复游戏事件
			}

			pop_ui->RemoveFromParent(); // 移除顶层UI
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
