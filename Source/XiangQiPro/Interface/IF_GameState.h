// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InterfaceCombinations.h"
#include "IF_GameState.generated.h"

// 调用暂停事件
#define EXEC_GAMEPAUSE() CALL_INTERFACE_EVENT(IF_GameState, GamePause)

// 调用恢复游戏事件
#define EXEC_GAMERESUME() CALL_INTERFACE_EVENT(IF_GameState, GameResume)

// 调用游戏结束事件
#define EXEC_GAMEOVER() CALL_INTERFACE_EVENT(IF_GameState, GameOver)

// 调用再次游戏事件
#define EXEC_PLAYAGAIN() CALL_INTERFACE_EVENT(IF_GameState, GamePlayAgain)

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIF_GameState : public UInterface
{
	GENERATED_BODY()
};

/**
 * 游戏状态回调函数类
 */
class XIANGQIPRO_API IIF_GameState
{
	GENERATED_BODY()

public:

	/*
	* The callback function for the game pause event. 
	* Please add "IIF_GameState::GamePause(OwnerObject)" in the bottom of your virtual function. Otherwise, the function in blueprint that inherit your native class won't be called.
	* @param OwnerObject The owner of interface.
	*/
	virtual void GamePause(UObject* OwnerObject);

	/*
	* The callback function for the game resume event.
	* Please add "IIF_GameState::GameResume(OwnerObject)" in the bottom of your virtual function. Otherwise, the function in blueprint that inherit your native class won't be called.
	* @param OwnerObject The owner of interface.
	*/
	virtual void GameResume(UObject* OwnerObject);

	/*
	* The callback function for the game over event.
	* Please add "IIF_GameState::GameOver(OwnerObject)" in the bottom of your virtual function. Otherwise, the function in blueprint that inherit your native class won't be called.
	* @param OwnerObject The owner of interface.
	*/
	virtual void GameOver(UObject* OwnerObject);


	/*
	* The callback function for the game over event.
	* Please add "IIF_GameState::GamePlayAgain(OwnerObject)" in the bottom of your virtual function. Otherwise, the function in blueprint that inherit your native class won't be called.
	* @param OwnerObject The owner of interface.
	*/
	virtual void GamePlayAgain(UObject* OwnerObject);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GamePause();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GameResume();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GameOver();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GamePlayAgain();
};
