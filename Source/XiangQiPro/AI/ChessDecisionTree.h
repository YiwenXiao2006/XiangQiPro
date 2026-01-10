// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"// 简单的决策树节点
struct ChessDecisionTreeNode
{
    int32 FeatureIndex = -1;      // 划分特征索引
    float Threshold = 0.0f;       // 划分阈值
    TSharedPtr<ChessDecisionTreeNode> LeftChild;  // 左子树
    TSharedPtr<ChessDecisionTreeNode> RightChild; // 右子树
    TArray<float> Predictions;    // 叶子节点的预测值
    bool bIsLeaf = false;         // 是否为叶子节点
};

// 决策树类
class XIANGQIPRO_API ChessDecisionTree
{
public:
    ChessDecisionTree(int32 MaxDepth = 10, int32 MinSamplesSplit = 2);

    // 训练决策树
    void Train(const TArray<TArray<float>>& Features, const TArray<TArray<float>>& Labels);

    // 预测
    TArray<float> Predict(const TArray<float>& Features) const;

    // 保存决策树
    bool SaveToFile(const FString& FilePath) const;

    // 加载决策树
    bool LoadFromFile(const FString& FilePath);

    // 计算信息增益
    float CalculateInformationGain(
        const TArray<TArray<float>>& Features,
        const TArray<TArray<float>>& Labels,
        int32 FeatureIndex,
        float Threshold);

    float CalculateGiniImpurity(const TArray<TArray<float>>& Labels);

private:
    // 递归构建决策树
    TSharedPtr<ChessDecisionTreeNode> BuildTree(
        const TArray<TArray<float>>& Features,
        const TArray<TArray<float>>& Labels,
        int32 Depth);

    // 获取最佳划分
    std::pair<int32, float> GetBestSplit(
        const TArray<TArray<float>>& Features,
        const TArray<TArray<float>>& Labels);

    // 序列化节点
    void SerializeNode(TSharedPtr<ChessDecisionTreeNode> Node, FMemoryWriter& Writer) const;

    // 反序列化节点
    TSharedPtr<ChessDecisionTreeNode> DeserializeNode(FMemoryReader& Reader);

private:
    TSharedPtr<ChessDecisionTreeNode> Root;
    int32 MaxDepth = 10;
    int32 MinSamplesSplit = 2;
    int32 NumFeatures = 0;
    int32 NumClasses = 0;
};