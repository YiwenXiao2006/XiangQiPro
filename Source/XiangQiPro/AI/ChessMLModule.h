// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessMLModule.generated.h"

// 机器学习数据结构
USTRUCT(BlueprintType)
struct FChessMLData
{
    GENERATED_BODY()

    UPROPERTY()
    FString BoardFen;           // 棋局FEN表示

    UPROPERTY()
    FString Move;              // 走的棋步

    UPROPERTY()
    float Score;               // 局面评分

    UPROPERTY()
    int32 Depth;               // 搜索深度

    UPROPERTY()
    FDateTime Timestamp;       // 时间戳
};

// 训练结果
USTRUCT(BlueprintType)
struct FMLTrainingResult
{
    GENERATED_BODY()

    UPROPERTY()
    int32 EpochsTrained = 0;

    UPROPERTY()
    float Accuracy = 0.0f;

    UPROPERTY()
    float Loss = 0.0f;

    UPROPERTY()
    FDateTime LastTrainingTime;
};

// 机器学习模块的具体实现
UCLASS(BlueprintType, Blueprintable)
class XIANGQIPRO_API UChessMLModule : public UObject
{
    GENERATED_BODY()

public:
    UChessMLModule();

    // 初始化
    virtual bool Initialize();

    // 保存训练数据
    virtual void SaveTrainingData(const FString& BoardFen, const FString& Move, float Score, int32 Depth);

    // 开始训练
    virtual bool StartTraining(int32 Epochs = 100);

    // 预测最佳走法
    virtual FString PredictBestMove(const FString& BoardFen, const TArray<FString>& ValidMoves);

    // 保存模型
    virtual bool SaveModel(const FString& ModelName);

    // 加载模型
    virtual bool LoadModel(const FString& ModelName);

    bool SaveNeuralNetwork(const FString& FilePath);

    bool LoadNeuralNetwork(const FString& FilePath);

    bool SaveTrainingResult(const FString& FilePath);

    bool LoadTrainingResult(const FString& FilePath);

    FMLTrainingResult GetTrainingStatus() const;

    bool IsTrained() const;

    void ClearData();

private:
    // 神经网络预测
    FString PredictWithNeuralNetwork(const FString& BoardFen, const TArray<FString>& ValidMoves);

    // 决策树预测
    FString PredictWithDecisionTree(const FString& BoardFen, const TArray<FString>& ValidMoves);

    // 加载训练数据
    void LoadTrainingDataFromFile();

    // 保存训练数据到文件
    void SaveTrainingDataToFile();

    void UpdateFeatureCache();

    // 训练神经网络
    void TrainNeuralNetwork(int32 Epochs);

    // 训练决策树
    void TrainDecisionTree(int32 Epochs);

    TArray<float> ForwardPass(const TArray<float>& Input, TArray<TArray<float>>& LayerOutputs);

    void BackwardPass(const TArray<float>& Input, const TArray<float>& Target, const TArray<TArray<float>>& LayerOutputs, const TArray<float>& Output);

    void InitializeNeuralNetwork();

    // 简化棋盘表示
    TArray<float> ConvertBoardToFeatures(const FString& BoardFen);

    int32 GetPieceTypeFromChar(TCHAR Char);

    // 简化走法表示
    TArray<float> ConvertMoveToFeatures(const FString& Move);

    float CalculateLoss(const TArray<float>& Output, const TArray<float>& Target);

    void ApplyReLU(TArray<float>& Data);

    void ApplySoftmax(TArray<float>& Data);

protected:
    TArray<FChessMLData> TrainingData;
    FMLTrainingResult TrainingResult;
    bool bIsTraining = false;
    bool bIsInitialized = false;
    FString ModelSavePath = "MLModels/";
    FString DataSavePath = "TrainingData/";

private:
    TArray<TArray<float>> NeuralNetworkWeights;
    TArray<TArray<float>> NeuralNetworkBiases;

    // 决策树
    TSharedPtr<class ChessDecisionTree> DecisionTree;

    // 训练数据缓存
    TArray<TArray<float>> FeatureCache;
    TArray<TArray<float>> LabelCache;

    int32 TrainingBatchSize = 32;
    float LearningRate = 0.001f;
};
