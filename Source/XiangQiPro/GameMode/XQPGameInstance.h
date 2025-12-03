// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "../Util/DoOnce.h"
#include "../Util/Logger.h"
#include "../UI/UI_LoadingScreen.h"
#include "../UI/UIManager.h"
#include "MoviePlayer.h"

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "XQPGameInstance.generated.h"

/**
 * ”Œœ∑ µ¿˝¿‡
 */
UCLASS()
class XIANGQIPRO_API UXQPGameInstance : public UGameInstance
{
	GENERATED_BODY()

private:

	FDoOnce<void(FLoadingScreenAttributes&)> FirstLoad;

	bool IsLoadingLevel = false;

	UUIManager* UIManager;

public:

	UXQPGameInstance();

	UPROPERTY()
	UUserWidget* CurrentWidget;

	// loading widget
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUI_LoadingScreen> LoadingWidget;

	// loading widget
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> FirstLoadingWidget;

public:

	virtual void Init() override;

	void BeginLoadMap(const FString& MapName);

	void EndLoadMap(UWorld* LoadedWorld);
	
};
