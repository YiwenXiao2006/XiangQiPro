// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "XiangQiPro/Interface/IF_GameState.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera2PActor.generated.h"

class AXQP_HUD;
class UUI_Battle2P_Base;
class UUI_InGamePause;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API ACamera2PActor : public APawn, public IIF_GameState
{
	GENERATED_BODY()

private:

	AXQP_HUD* HUD;

private:

    // 触摸状态
    bool bIsSingleTouchActive;
    bool bIsTwoFingerTouchActive;
    FVector2D InitialTouchLocation;
    FVector2D PreviousTouchLocation;

    // 双指触摸状态
    FVector2D FirstFingerLocation;
    FVector2D SecondFingerLocation;
    float InitialFingerDistance;

    // 目标值
    FRotator TargetRotation;

    bool bIsRotatingCamera;

    UUI_Battle2P_Base* BaseUI;

public:

	ACamera2PActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* SpringArm;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* GameCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
    UAudioComponent* BattleAudio;

    UPROPERTY(EditAnywhere, Category = "Music")
    USoundBase* BattleMusic;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float TargetArmLength = 90.f;

    // 基准FOV
    UPROPERTY(EditAnywhere, Category = "Camera")
    float BaseFOV = 90.f;

    // 平滑插值速度
    UPROPERTY(EditAnywhere, Category = "Touch Control")
    float RotationInterpSpeed = 8.0f;

    UPROPERTY(EditAnywhere, Category = "Touch Control")
    float ZoomInterpSpeed = 5.0f;

    // 鼠标滚轮缩放灵敏度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MouseWheelZoomSensitivity = 50.0f;

    // 触摸控制参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float RotationSensitivity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float ZoomSensitivity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MinZoomDistance = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxZoomDistance = 200.0f; 
    
    // 最小捏合距离，避免误触
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float MinPinchDistance = 20.0f;

    // 旋转控制变量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float RotationSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MinPitchAngle = -89.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxPitchAngle = -10.0f;

protected:

	virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 初始化相机
    void InitCamera();

    // 初始化音频
    void InitAudio();

    // 初始化用户界面
    void InitUI();

public:

    virtual void Tick(float DeltaTime) override;

    virtual void GamePause(UObject* OwnerObject) override;

    virtual void GameResume(UObject* OwnerObject) override;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // 输入处理函数
    void OnMouseRightButtonPressed();
    void OnMouseRightButtonReleased();
    void OnMouseX(float Value);
    void OnMouseY(float Value);
    void OnMouseWheel(float Value);

    // 触摸输入处理
    void OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location);
    void OnTouchMove(ETouchIndex::Type FingerIndex, FVector Location);
    void OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location);

    // 双指手势处理
    void HandleTwoFingerGesture();

private:

    // 双指捏合距离是否合法
    bool IsValidTwoFingerGesture();
	
};
