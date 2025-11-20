// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera2PActor.generated.h"

class AXQP_HUD;

/**
 * 
 */
UCLASS()
class XIANGQIPRO_API ACamera2PActor : public APawn
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

public:

	ACamera2PActor();

    // 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* GameCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    UAudioComponent* BattleAudio;

    UPROPERTY(EditAnywhere, Category = "Music")
    USoundBase* BattleMusic;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float TargetArmLength;

    // 平滑插值速度
    UPROPERTY(EditAnywhere, Category = "Touch Control")
    float RotationInterpSpeed = 8.0f;

    UPROPERTY(EditAnywhere, Category = "Touch Control")
    float ZoomInterpSpeed = 5.0f;

    // 触摸控制参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float RotationSensitivity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float ZoomSensitivity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float MinZoomDistance = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
    float MaxZoomDistance = 2000.0f;

    // 旋转控制变量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float RotationSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MinPitchAngle = -89.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxPitchAngle = -10.0f;

protected:

	virtual void BeginPlay() override;

public:

    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // 输入处理函数
    void OnMouseRightButtonPressed();
    void OnMouseRightButtonReleased();
    void OnMouseX(float Value);
    void OnMouseY(float Value);

    // 触摸输入处理
    void OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location);
    void OnTouchMove(ETouchIndex::Type FingerIndex, FVector Location);
    void OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location);

    // 双指手势处理
    void HandleTwoFingerGesture();
	
};
