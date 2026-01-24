// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "InterfaceCombinations.h"

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IF_EndingGame.generated.h"

#define EXEC_ONENDINGGAMESTART(INDEX) CALL_INTERFACE_EVENT_PARAM(IF_EndingGame, OnEndingGameStart, INDEX)
#define EXEC_ONJUESHA() CALL_INTERFACE_EVENT(IF_EndingGame, OnJueSha)

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIF_EndingGame : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XIANGQIPRO_API IIF_EndingGame
{
	GENERATED_BODY()

public:

	virtual void OnEndingGameStart(UObject* OwnerObject, int32 Index);

	virtual void OnJueSha(UObject* OwnerObject);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnEndingGameStart(int32 Index);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnJueSha();
};
