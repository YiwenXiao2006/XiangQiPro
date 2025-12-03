// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "XQPPlayerController.generated.h"

typedef AXQPPlayerController PC;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API AXQPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	UFUNCTION()
	void OnEscapePressed();
	
};
