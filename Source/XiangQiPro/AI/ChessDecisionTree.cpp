// Copyright 2026 Ultimate Player All Rights Reserved.


#include "ChessDecisionTree.h"
#include "Misc/FileHelper.h"

ChessDecisionTree::ChessDecisionTree(int32 InMaxDepth, int32 InMinSamplesSplit)
    : MaxDepth(InMaxDepth)
    , MinSamplesSplit(InMinSamplesSplit)
{
}

void ChessDecisionTree::Train(const TArray<TArray<float>>& Features, const TArray<TArray<float>>& Labels)
{
    if (Features.Num() == 0 || Labels.Num() == 0)
    {
        return;
    }

    NumFeatures = Features[0].Num();
    NumClasses = Labels[0].Num();

    Root = BuildTree(Features, Labels, 0);
}

TArray<float> ChessDecisionTree::Predict(const TArray<float>& Features) const
{
    if (!Root.IsValid())
    {
        return TArray<float>();
    }

    TSharedPtr<ChessDecisionTreeNode> CurrentNode = Root;

    while (CurrentNode.IsValid() && !CurrentNode->bIsLeaf)
    {
        if (Features[CurrentNode->FeatureIndex] <= CurrentNode->Threshold)
        {
            CurrentNode = CurrentNode->LeftChild;
        }
        else
        {
            CurrentNode = CurrentNode->RightChild;
        }
    }

    return CurrentNode.IsValid() ? CurrentNode->Predictions : TArray<float>();
}

TSharedPtr<ChessDecisionTreeNode> ChessDecisionTree::BuildTree(const TArray<TArray<float>>& Features,
    const TArray<TArray<float>>& Labels,
    int32 Depth)
{
    if (Features.Num() < MinSamplesSplit || Depth >= MaxDepth)
    {
        // 创建叶子节点
        TSharedPtr<ChessDecisionTreeNode> LeafNode = MakeShared<ChessDecisionTreeNode>();
        LeafNode->bIsLeaf = true;

        // 计算平均预测值
        LeafNode->Predictions.SetNum(NumClasses);
        for (int32 i = 0; i < NumClasses; i++)
        {
            float Sum = 0.0f;
            for (const TArray<float>& Label : Labels)
            {
                Sum += Label[i];
            }
            LeafNode->Predictions[i] = Sum / Labels.Num();
        }

        return LeafNode;
    }

    // 寻找最佳划分
    auto BestSplit = GetBestSplit(Features, Labels);
    int32 BestFeature = BestSplit.first;
    float BestThreshold = BestSplit.second;

    if (BestFeature == -1)
    {
        // 无法划分，创建叶子节点
        return BuildTree(Features, Labels, Depth + 1);
    }

    // 根据最佳划分分割数据
    TArray<TArray<float>> LeftFeatures, RightFeatures;
    TArray<TArray<float>> LeftLabels, RightLabels;

    for (int32 i = 0; i < Features.Num(); i++)
    {
        if (Features[i][BestFeature] <= BestThreshold)
        {
            LeftFeatures.Add(Features[i]);
            LeftLabels.Add(Labels[i]);
        }
        else
        {
            RightFeatures.Add(Features[i]);
            RightLabels.Add(Labels[i]);
        }
    }

    // 如果分割后某一边没有数据，创建叶子节点
    if (LeftFeatures.Num() == 0 || RightFeatures.Num() == 0)
    {
        return BuildTree(Features, Labels, Depth + 1);
    }

    // 创建内部节点
    TSharedPtr<ChessDecisionTreeNode> Node = MakeShared<ChessDecisionTreeNode>();
    Node->FeatureIndex = BestFeature;
    Node->Threshold = BestThreshold;
    Node->bIsLeaf = false;

    // 递归构建左右子树
    Node->LeftChild = BuildTree(LeftFeatures, LeftLabels, Depth + 1);
    Node->RightChild = BuildTree(RightFeatures, RightLabels, Depth + 1);

    return Node;
}

std::pair<int32, float> ChessDecisionTree::GetBestSplit(const TArray<TArray<float>>& Features,
    const TArray<TArray<float>>& Labels)
{
    int32 BestFeature = -1;
    float BestThreshold = 0.0f;
    float BestGain = -MAX_FLT;

    // 尝试每个特征
    for (int32 FeatureIndex = 0; FeatureIndex < NumFeatures; FeatureIndex++)
    {
        // 收集该特征的所有可能值
        TArray<float> FeatureValues;
        for (const TArray<float>& Feature : Features)
        {
            FeatureValues.Add(Feature[FeatureIndex]);
        }

        // 排序并去重
        FeatureValues.Sort();
        TArray<float> UniqueValues;
        for (float Value : FeatureValues)
        {
            if (UniqueValues.Num() == 0 || Value != UniqueValues.Last())
            {
                UniqueValues.Add(Value);
            }
        }

        // 尝试每个可能的分割点
        for (int32 i = 0; i < UniqueValues.Num() - 1; i++)
        {
            float Threshold = (UniqueValues[i] + UniqueValues[i + 1]) / 2.0f;
            float Gain = CalculateInformationGain(Features, Labels, FeatureIndex, Threshold);

            if (Gain > BestGain)
            {
                BestGain = Gain;
                BestFeature = FeatureIndex;
                BestThreshold = Threshold;
            }
        }
    }

    return { BestFeature, BestThreshold };
}

float ChessDecisionTree::CalculateInformationGain(const TArray<TArray<float>>& Features,
    const TArray<TArray<float>>& Labels,
    int32 FeatureIndex, float Threshold)
{
    // 计算父节点的基尼不纯度
    float ParentGini = CalculateGiniImpurity(Labels);

    // 根据阈值分割数据
    TArray<TArray<float>> LeftLabels, RightLabels;

    for (int32 i = 0; i < Features.Num(); i++)
    {
        if (Features[i][FeatureIndex] <= Threshold)
        {
            LeftLabels.Add(Labels[i]);
        }
        else
        {
            RightLabels.Add(Labels[i]);
        }
    }

    // 计算加权平均基尼不纯度
    float WeightedGini = 0.0f;
    WeightedGini += (LeftLabels.Num() / (float)Labels.Num()) * CalculateGiniImpurity(LeftLabels);
    WeightedGini += (RightLabels.Num() / (float)Labels.Num()) * CalculateGiniImpurity(RightLabels);

    // 信息增益 = 父节点不纯度 - 子节点加权不纯度
    return ParentGini - WeightedGini;
}

float ChessDecisionTree::CalculateGiniImpurity(const TArray<TArray<float>>& Labels)
{
    if (Labels.Num() == 0) return 0.0f;

    // 计算每个类别的概率
    TArray<float> ClassProbabilities;
    ClassProbabilities.SetNum(NumClasses);

    for (const TArray<float>& Label : Labels)
    {
        for (int32 i = 0; i < NumClasses; i++)
        {
            ClassProbabilities[i] += Label[i];
        }
    }

    // 归一化
    for (int32 i = 0; i < NumClasses; i++)
    {
        ClassProbabilities[i] /= Labels.Num();
    }

    // 计算基尼不纯度: 1 - sum(p_i^2)
    float Gini = 1.0f;
    for (float Prob : ClassProbabilities)
    {
        Gini -= Prob * Prob;
    }

    return Gini;
}

bool ChessDecisionTree::SaveToFile(const FString& FilePath) const
{
    TArray<uint8> Data;
    FMemoryWriter Writer(Data);

    // 保存树结构
    SerializeNode(Root, Writer);

    return FFileHelper::SaveArrayToFile(Data, *FilePath);
}

bool ChessDecisionTree::LoadFromFile(const FString& FilePath)
{
    TArray<uint8> Data;
    if (!FFileHelper::LoadFileToArray(Data, *FilePath))
    {
        return false;
    }

    FMemoryReader Reader(Data);
    Root = DeserializeNode(Reader);

    return Root.IsValid();
}

void ChessDecisionTree::SerializeNode(TSharedPtr<ChessDecisionTreeNode> Node, FMemoryWriter& Writer) const
{
    if (!Node.IsValid())
    {
        int32 IsNull = 1;
        Writer << IsNull;
        return;
    }

    int32 IsNull = 0;
    Writer << IsNull;
    Writer << Node->bIsLeaf;
    Writer << Node->FeatureIndex;
    Writer << Node->Threshold;

    if (Node->bIsLeaf)
    {
        int32 NumPredictions = Node->Predictions.Num();
        Writer << NumPredictions;
        for (float Prediction : Node->Predictions)
        {
            Writer << Prediction;
        }
    }
    else
    {
        SerializeNode(Node->LeftChild, Writer);
        SerializeNode(Node->RightChild, Writer);
    }
}

TSharedPtr<ChessDecisionTreeNode> ChessDecisionTree::DeserializeNode(FMemoryReader& Reader)
{
    int32 IsNull = 0;
    Reader << IsNull;
    if (IsNull == 1)
    {
        return nullptr;
    }

    TSharedPtr<ChessDecisionTreeNode> Node = MakeShared<ChessDecisionTreeNode>();
    Reader << Node->bIsLeaf;
    Reader << Node->FeatureIndex;
    Reader << Node->Threshold;

    if (Node->bIsLeaf)
    {
        int32 NumPredictions = 0;
        Reader << NumPredictions;
        Node->Predictions.SetNum(NumPredictions);
        for (int32 i = 0; i < NumPredictions; i++)
        {
            Reader << Node->Predictions[i];
        }
    }
    else
    {
        Node->LeftChild = DeserializeNode(Reader);
        Node->RightChild = DeserializeNode(Reader);
    }

    return Node;
}