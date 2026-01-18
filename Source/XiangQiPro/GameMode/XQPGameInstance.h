// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "XiangQiPro/Util/DoOnce.h"
#include "XiangQiPro/Util/Logger.h"
#include "XiangQiPro/UI/Util/UI_LoadingScreen.h"
#include "XiangQiPro/UI/UIManager.h"
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

	UUIManager* UIManager;

public:

	UXQPGameInstance();

	UPROPERTY(BlueprintReadWrite)
	uint8 AIDifficulty;

	UPROPERTY()
	UUserWidget* CurrentWidget;

	UPROPERTY(BlueprintReadOnly, Category = "MoviePlayer")
	bool bIsLoadingLevel = false;

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

	UFUNCTION(BlueprintCallable, Category = "MoviePlayer")
	void StopMovie();
	
};
