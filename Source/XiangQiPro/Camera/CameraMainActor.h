// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CineCameraComponent.h"
#include "CameraMainActor.generated.h"

UCLASS()
class XIANGQIPRO_API ACameraMainActor : public AActor
{
	GENERATED_BODY()

public:
	// 相机聚焦的物体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	AActor* FocusTarget;

	// 相机与目标的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraDistance;

	// 相机高度偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraHeightOffset;

	// 鼠标灵敏度，值越小晃动幅度越小
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MouseSensitivity = 0.1f;

	// 最大水平偏移角度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MaxHorizontalOffset = 5.0f;

	// 最大垂直偏移角度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MaxVerticalOffset = 3.0f;

	// 镜头旋转偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float RotationZOffset = 3.0f;

public:
	// Sets default values for this actor's properties
	ACameraMainActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 更新相机聚焦
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void UpdateCameraFocus();

	// 为玩家启用这个相机
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void EnableForPlayer(int32 PlayerIndex);

	// 设置CineCamera参数
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetupCineCameraParameters();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 设置新的聚焦目标
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFocusTarget(AActor* NewTarget);

	// 获取CineCamera组件
	UFUNCTION(BlueprintCallable, Category = "Camera")
	class UCineCameraComponent* GetCineCameraComponent() const { return CineCameraComponent; }

private:
	// CineCamera组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCineCameraComponent* CineCameraComponent;

	// 场景根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	// 鼠标位置变量
	float CurrentMouseX = 0.0f;
	float CurrentMouseY = 0.0f;
	float TargetMouseX = 0.0f;
	float TargetMouseY = 0.0f;

	// 基础相机角度（基于初始位置计算）
	float BaseHorizontalAngle = 0.0f;
	float BaseVerticalAngle = 0.0f;

	// 是否已启用鼠标输入
	bool bMouseInputEnabled = false;

	// 添加鼠标输入处理函数
	void SetupMouseInput();
	void HandleMouseMovement(float DeltaTime);
	void CalculateBaseAngles();
};