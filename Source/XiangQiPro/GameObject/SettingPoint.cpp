// Copyright 2026 Ultimate Player All Rights Reserved.


#include "SettingPoint.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"

#include "../Chess/Chesses.h"
#include "../GameObject/ChessBoard2P.h"
#include "../GameMode/XQPGameStateBase.h"
#include "../Util/ObjectManager.h"

// Sets default values
ASettingPoint::ASettingPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	NS_SettingPoint = OM::GetConstructorObject<UNiagaraSystem>(PATH_NS_SETTINGPOINT);

	PointNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PointNiagara"));
	PointNiagara->SetAsset(NS_SettingPoint);
	PointNiagara->bAutoActivate = false;
	PointNiagara->SetActive(false);
	RootComponent = PointNiagara;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetSphereRadius(2);
	Sphere->SetupAttachment(PointNiagara);
	Sphere->OnClicked.AddDynamic(this, &ASettingPoint::OnComponentClicked);
	Sphere->OnBeginCursorOver.AddDynamic(this, &ASettingPoint::OnComponentBeginCursorOver);
	Sphere->OnEndCursorOver.AddDynamic(this, &ASettingPoint::OnComponentEndCursorOver);
}

// Called when the game starts or when spawned
void ASettingPoint::BeginPlay()
{
	Super::BeginPlay();
	GameState = Cast<GS>(GetWorld()->GetGameState());
	DefaultWorldPosition = GetActorLocation(); // 获取默认世界坐标
}

// Called every frame
void ASettingPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASettingPoint::OnComponentClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	GameState->ApplyMove2P(TargetChess, FChessMove2P(TargetChess->GetSimpPosition(), Position2P));
}

void ASettingPoint::OnComponentBeginCursorOver(UPrimitiveComponent* TouchedComponent)
{
	PointNiagara->SetVectorParameter(FName(TEXT("RayColor")), FVector(1, 0, 0));
}

void ASettingPoint::OnComponentEndCursorOver(UPrimitiveComponent* TouchedComponent)
{
	PointNiagara->SetVectorParameter(FName(TEXT("RayColor")), FVector(1, 1, 1));
}

void ASettingPoint::SetActivate(bool bInActive)
{
	bActive = bInActive;
	PointNiagara->SetActive(bActive);
	if (bActive)
	{
		PointNiagara->Activate();
		PointNiagara->SetHiddenInGame(false);
		Sphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); // 开始相应鼠标

		if (GameState->GetChessBoard2P()->Board[Position2P.X][Position2P.Y] != nullptr) // 可以吃子
		{
			SetActorLocation(DefaultWorldPosition + FVector(0, 0, 2.5f)); // 向上偏移,到棋子顶部
		}
	}
	else
	{
		PointNiagara->Deactivate();
		PointNiagara->SetHiddenInGame(true);
		SetActorLocation(DefaultWorldPosition); // 恢复默认位置
		Sphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // 禁止相应鼠标
	}
}

void ASettingPoint::SetTargetChess(TWeakObjectPtr<AChesses> targetChess)
{
	TargetChess = targetChess;
}

TWeakObjectPtr<AChesses> ASettingPoint::GetTargetChess()
{
	return TargetChess;
}

FVector2D ASettingPoint::GetPosition2P()
{
	return Position2P;
}

void ASettingPoint::SetPosition2P(FVector2D InPosition2P)
{
	Position2P = InPosition2P;
}

