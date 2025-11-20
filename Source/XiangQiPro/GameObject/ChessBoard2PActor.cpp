// Copyright 2026 Ultimate Player All Rights Reserved.


#include "ChessBoard2PActor.h"
#include "Components/StaticMeshComponent.h"

#include "ChessBoard2P.h"
#include "../Chess/AllChessHeader.h"
#include "../GameMode/XQPGameStateBase.h"
#include "../GameObject/SettingPoint.h"
#include "../Util/ObjectManager.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
AChessBoard2PActor::AChessBoard2PActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ChessBoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChessBoardMeshComponent"));
	ChessBoardMesh->SetStaticMesh(OM::GetConstructorObject<UStaticMesh>(PATH_SM_CHESSBOARD_2P));
}

// Called when the game starts or when spawned
void AChessBoard2PActor::BeginPlay()
{
	Super::BeginPlay();
    AXQPGameStateBase* GameState = Cast<AXQPGameStateBase>(GetWorld()->GetGameState());
    if (GameState)
    {
        GameState->Start2PGame(this);
    }
}

// Called every frame
void AChessBoard2PActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AChessBoard2PActor::GenerateChesses(TWeakObjectPtr<UChessBoard2P> board2P)
{
    if (!board2P.IsValid())
    {
        ULogger::LogError(TEXT("Can't generate chesses, because board2P is nullptr!"));
        return;
    }

    TArray<TPair<int32, int32>> Indexs = { {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8}, {2, 1}, {2, 7}, {3, 0}, {3, 2}, {3, 4}, {3, 6}, {3, 8},
                                           {9, 0}, {9, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5}, {9, 6}, {9, 7}, {9, 8}, {7, 1}, {7, 7}, {6, 0}, {6, 2}, {6, 4}, {6, 6}, {6, 8} };
    
    TArray<TSubclassOf<AChesses>> Classes = { 
    AChess_Jv::StaticClass(), AChess_Ma::StaticClass(), AChess_Xiang::StaticClass(), AChess_Shi::StaticClass(), AChess_Jiang::StaticClass(), AChess_Shi::StaticClass(), AChess_Xiang::StaticClass(), AChess_Ma::StaticClass(), AChess_Jv::StaticClass(), 
    AChess_Pao::StaticClass(), AChess_Pao::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(),
    AChess_Jv::StaticClass(), AChess_Ma::StaticClass(), AChess_Xiang::StaticClass(), AChess_Shi::StaticClass(), AChess_Jiang::StaticClass(), AChess_Shi::StaticClass(), AChess_Xiang::StaticClass(), AChess_Ma::StaticClass(), AChess_Jv::StaticClass(),
    AChess_Pao::StaticClass(), AChess_Pao::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass(), AChess_Bing::StaticClass() };

#define RED EChessColor::RED
#define BLACK EChessColor::BLACK

    TArray<EChessColor> Colors = { RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, 
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };

    for (int32 i = 0; i < 32; i++)
    {
        FVector WorldLoc = board2P->BoardLocs[Indexs[i].Key][Indexs[i].Value]; // 世界坐标
        FVector2D SimpLoc = FVector2D(Indexs[i].Key, Indexs[i].Value); // 棋盘坐标
        FTransform Transform(WorldLoc);

        // 生成棋子
        AChesses* Chess = Cast<AChesses>(
            UGameplayStatics::BeginDeferredActorSpawnFromClass(
                GetWorld(),
                Classes[i],  // 用指定类型生成棋子
                Transform,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn
            )
        );
        Chess->Init(Colors[i], SimpLoc, board2P); // 初始化棋子
        Chess->FinishSpawning(Transform);

        // 将棋子保存到棋盘中
        board2P->AllChess[Indexs[i].Key][Indexs[i].Value] = Chess;
    }

    for (int32 i = 0; i < 10; i++)
    {
        for (int32 j = 0; j < 9; j++)
        {
            FVector WorldLoc = board2P->BoardLocs[i][j]; // 世界坐标
            WorldLoc.Z = 80; // 和棋子的高度不一样
            FTransform Transform(WorldLoc);

            // 生成置棋位置Actor
            ASettingPoint* SettingPoint = Cast<ASettingPoint>(
                UGameplayStatics::BeginDeferredActorSpawnFromClass(
                    GetWorld(),
                    ASettingPoint::StaticClass(),
                    Transform,
                    ESpawnActorCollisionHandlingMethod::AlwaysSpawn
                )
            );
            SettingPoint->SetPosition2P(FVector2D(i, j));
            SettingPoint->FinishSpawning(Transform);

            // 保存到棋盘中
            board2P->SettingPoints[i][j] = SettingPoint;
        }
    }
}

