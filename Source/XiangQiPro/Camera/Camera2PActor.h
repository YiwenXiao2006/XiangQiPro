// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera2PActor.generated.h"

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API ACamera2PActor : public ACameraActor
{
	GENERATED_BODY()

public:

	ACamera2PActor();

	UPROPERTY(EditAnywhere)
	UCameraComponent* GameCamera;
	
};
