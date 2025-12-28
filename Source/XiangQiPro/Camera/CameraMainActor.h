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
	// ����۽�������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	AActor* FocusTarget;

	// �����Ŀ��ľ���
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraDistance;

	// ����߶�ƫ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraHeightOffset;

	// �������ȣ�ֵԽС�ζ�����ԽС
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MouseSensitivity = 0.1f;

	// ���ˮƽƫ�ƽǶ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MaxHorizontalOffset = 5.0f;

	// ���ֱƫ�ƽǶ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MaxVerticalOffset = 3.0f;

	// ��ͷ��תƫ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float RotationZOffset = 3.0f;

public:	
	// Sets default values for this actor's properties
	ACameraMainActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ��������۽�
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void UpdateCameraFocus();

	// Ϊ�������������
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void EnableForPlayer(int32 PlayerIndex);

	// ����CineCamera����
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetupCineCameraParameters();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// �����µľ۽�Ŀ��
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFocusTarget(AActor* NewTarget);

	// ��ȡCineCamera���
	UFUNCTION(BlueprintCallable, Category = "Camera")
	class UCineCameraComponent* GetCineCameraComponent() const { return CineCameraComponent; }

private:
	// CineCamera���
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCineCameraComponent* CineCameraComponent;

	// ���������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	// ���λ�ñ���
	float CurrentMouseX = 0.0f;
	float CurrentMouseY = 0.0f;
	float TargetMouseX = 0.0f;
	float TargetMouseY = 0.0f;

	// ��������Ƕȣ����ڳ�ʼλ�ü��㣩
	float BaseHorizontalAngle = 0.0f;
	float BaseVerticalAngle = 0.0f;

	// �Ƿ��������������
	bool bMouseInputEnabled = false;

	// ���������봦�����
	void SetupMouseInput();
	void HandleMouseMovement(float DeltaTime);
	void CalculateBaseAngles();
};
