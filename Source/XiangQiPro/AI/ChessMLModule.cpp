// Copyright 2026 Ultimate Player All Rights Reserved.


#include "ChessMLModule.h"
#include "ChessDecisionTree.h"

#include "XiangQiPro/Util/AsyncWorker.h"
#include "XiangQiPro/Util/Logger.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UChessMLModule::UChessMLModule()
{
    ModelSavePath = TEXT("Saved/ChessML/Models/");
    DataSavePath = TEXT("Saved/ChessML/Data/");
}

bool UChessMLModule::Initialize()
{
    // 创建保存目录
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    FString ModelPath = FPaths::ProjectSavedDir() + ModelSavePath;
    FString DataPath = FPaths::ProjectSavedDir() + DataSavePath;

    if (!PlatformFile.DirectoryExists(*ModelPath))
    {
        PlatformFile.CreateDirectoryTree(*ModelPath);
    }

    if (!PlatformFile.DirectoryExists(*DataPath))
    {
        PlatformFile.CreateDirectoryTree(*DataPath);
    }

    // 初始化决策树
    DecisionTree = MakeShared<ChessDecisionTree>(10, 2);

    // 加载已有训练数据
    LoadTrainingDataFromFile();

    bIsInitialized = true;
    ULogger::Log(FString::Printf(TEXT("Chess ML Module Initialized with %d training samples"), TrainingData.Num()));
    return true;
}

void UChessMLModule::SaveTrainingData(const FString& BoardFen, const FString& Move, float Score, int32 Depth)
{
    FChessMLData Data;
    Data.BoardFen = BoardFen;
    Data.Move = Move;
    Data.Score = Score;
    Data.Depth = Depth;
    Data.Timestamp = FDateTime::Now();

    TrainingData.Add(Data);

    // 限制数据量
    if (TrainingData.Num() > 10000)
    {
        TrainingData.RemoveAt(0, TrainingData.Num() - 10000);
    }

    // 保存到文件
    SaveTrainingDataToFile();

    // 更新特征缓存
    UpdateFeatureCache();
}

bool UChessMLModule::StartTraining(int32 Epochs)
{
    if (TrainingData.Num() < 100)
    {
        ULogger::Log(FString::Printf(TEXT("训练数据不足，需要至少100条数据，当前只有%d条"), TrainingData.Num()));
        return false;
    }

    if (bIsTraining)
    {
        ULogger::Log(UTF8_TO_TCHAR("训练正在进行中，请等待当前训练完成"));
        return false;
    }

    bIsTraining = true;
    TWeakObjectPtr<UChessMLModule> WeakThis;

    // 在后台线程中进行训练
    UAsyncWorker::CreateAndStartWorker(
        [WeakThis, Epochs](UAsyncWorker* WorkerInstance)
        {
            if (!WeakThis.IsValid()) return;

            // 获取最佳移动方式和要移动的棋子
            ULogger::Log(FString::Printf(TEXT("开始训练，数据量: %d, 轮次: %d"), WeakThis->TrainingData.Num(), Epochs));

            // 转换为特征向量
            WeakThis->UpdateFeatureCache();

            if (WeakThis->FeatureCache.Num() == 0)
            {
                ULogger::LogError(UTF8_TO_TCHAR("特征提取失败，无法训练"));
                WeakThis->bIsTraining = false;
                return;
            }

            // 训练神经网络
            WeakThis->TrainNeuralNetwork(Epochs);

            // 训练决策树
            WeakThis->TrainDecisionTree(Epochs);

            WeakThis->bIsTraining = false;
            WeakThis->TrainingResult.EpochsTrained += Epochs;
            WeakThis->TrainingResult.LastTrainingTime = FDateTime::Now();

            ULogger::Log(FString::Printf(TEXT("训练完成，总训练轮次: %d"), WeakThis->TrainingResult.EpochsTrained));
        },
        [WeakThis](EAsyncWorkerState State)
        {
            if (WeakThis.IsValid())
                WeakThis->SaveModel(TEXT("ChineseChess"));
        }
    );

    return true;
}

FString UChessMLModule::PredictBestMove(const FString& BoardFen, const TArray<FString>& ValidMoves)
{
    if (!bIsInitialized || ValidMoves.Num() == 0)
    {
        return TEXT("");
    }

    // 如果有神经网络模型，优先使用
    if (NeuralNetworkWeights.Num() > 0)
    {
        FString NeuralNetMove = PredictWithNeuralNetwork(BoardFen, ValidMoves);
        if (!NeuralNetMove.IsEmpty())
        {
            return NeuralNetMove;
        }
    }

    // 否则使用决策树
    if (DecisionTree.IsValid())
    {
        FString DecisionTreeMove = PredictWithDecisionTree(BoardFen, ValidMoves);
        if (!DecisionTreeMove.IsEmpty())
        {
            return DecisionTreeMove;
        }
    }

    // 都没有则返回空字符串，让传统AI处理
    return TEXT("");
}

bool UChessMLModule::SaveModel(const FString& ModelName)
{
    if (!bIsInitialized)
    {
        ULogger::LogError(TEXT("ML Module not initialized"));
        return false;
    }

    FString ModelPath = FPaths::ProjectSavedDir() + ModelSavePath + ModelName;

    // 保存神经网络
    bool bSuccess = SaveNeuralNetwork(ModelPath + "_NeuralNet.bin");

    // 保存决策树
    if (DecisionTree.IsValid())
    {
        bSuccess &= DecisionTree->SaveToFile(ModelPath + "_DecisionTree.bin");
    }

    // 保存训练结果
    bSuccess &= SaveTrainingResult(ModelPath + "_TrainingResult.json");

    if (bSuccess)
    {
        ULogger::Log(FString::Printf(TEXT("Model saved successfully: %s"), *ModelName));
    }
    else
    {
        ULogger::LogError(FString::Printf(TEXT("Failed to save model: %s"), *ModelName));
    }

    return bSuccess;
}

bool UChessMLModule::LoadModel(const FString& ModelName)
{
    FString ModelPath = FPaths::ProjectSavedDir() + ModelSavePath + ModelName;

    // 加载神经网络
    bool bSuccess = LoadNeuralNetwork(ModelPath + "_NeuralNet.bin");

    // 加载决策树
    if (DecisionTree.IsValid())
    {
        bSuccess &= DecisionTree->LoadFromFile(ModelPath + "_DecisionTree.bin");
    }

    // 加载训练结果
    bSuccess &= LoadTrainingResult(ModelPath + "_TrainingResult.json");

    if (bSuccess)
    {
        ULogger::Log(FString::Printf(TEXT("Model loaded successfully: %s"), *ModelName));
        bIsInitialized = true;
    }
    else
    {
        ULogger::LogError(FString::Printf(TEXT("Failed to load model: %s"), *ModelName));
    }

    return bSuccess;
}

// 神经网络保存/加载辅助函数
bool UChessMLModule::SaveNeuralNetwork(const FString& FilePath)
{
    TArray<uint8> Data;
    FMemoryWriter Writer(Data);

    // 保存网络结构
    int32 NumLayers = NeuralNetworkWeights.Num();
    Writer << NumLayers;

    for (int32 i = 0; i < NumLayers; i++)
    {
        int32 NumWeights = NeuralNetworkWeights[i].Num();
        Writer << NumWeights;

        for (float Weight : NeuralNetworkWeights[i])
        {
            Writer << Weight;
        }

        int32 NumBiases = NeuralNetworkBiases[i].Num();
        Writer << NumBiases;

        for (float Bias : NeuralNetworkBiases[i])
        {
            Writer << Bias;
        }
    }

    return FFileHelper::SaveArrayToFile(Data, *FilePath);
}

bool UChessMLModule::LoadNeuralNetwork(const FString& FilePath)
{
    TArray<uint8> Data;
    if (!FFileHelper::LoadFileToArray(Data, *FilePath))
    {
        return false;
    }

    FMemoryReader Reader(Data);

    int32 NumLayers = 0;
    Reader << NumLayers;

    NeuralNetworkWeights.SetNum(NumLayers);
    NeuralNetworkBiases.SetNum(NumLayers);

    for (int32 i = 0; i < NumLayers; i++)
    {
        int32 NumWeights = 0;
        Reader << NumWeights;

        NeuralNetworkWeights[i].SetNum(NumWeights);
        for (int32 j = 0; j < NumWeights; j++)
        {
            Reader << NeuralNetworkWeights[i][j];
        }

        int32 NumBiases = 0;
        Reader << NumBiases;

        NeuralNetworkBiases[i].SetNum(NumBiases);
        for (int32 j = 0; j < NumBiases; j++)
        {
            Reader << NeuralNetworkBiases[i][j];
        }
    }

    return true;
}

bool UChessMLModule::SaveTrainingResult(const FString& FilePath)
{
    TSharedPtr<FJsonObject> ResultObject = MakeShared<FJsonObject>();
    ResultObject->SetNumberField("EpochsTrained", TrainingResult.EpochsTrained);
    ResultObject->SetNumberField("Accuracy", TrainingResult.Accuracy);
    ResultObject->SetNumberField("Loss", TrainingResult.Loss);
    ResultObject->SetStringField("LastTrainingTime", TrainingResult.LastTrainingTime.ToString());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);

    return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

bool UChessMLModule::LoadTrainingResult(const FString& FilePath)
{
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> ResultObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);

    if (FJsonSerializer::Deserialize(Reader, ResultObject) && ResultObject.IsValid())
    {
        ResultObject->TryGetNumberField(TEXT("EpochsTrained"), TrainingResult.EpochsTrained);
        ResultObject->TryGetNumberField(TEXT("Accuracy"), TrainingResult.Accuracy);
        ResultObject->TryGetNumberField(TEXT("Loss"), TrainingResult.Loss);

        FString TimeString;
        if (ResultObject->TryGetStringField(TEXT("LastTrainingTime"), TimeString))
        {
            FDateTime::Parse(TimeString, TrainingResult.LastTrainingTime);
        }

        return true;
    }

    return false;
}

FMLTrainingResult UChessMLModule::GetTrainingStatus() const
{
    return TrainingResult;
}

bool UChessMLModule::IsTrained() const
{
    return TrainingResult.EpochsTrained > 0;
}

void UChessMLModule::ClearData()
{
    TrainingData.Empty();
    FeatureCache.Empty();
    LabelCache.Empty();
    TrainingResult = FMLTrainingResult();

    // 删除数据文件
    FString DataPath = FPaths::ProjectSavedDir() + DataSavePath + TEXT("TrainingData.json");
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.DeleteFile(*DataPath);

    ULogger::Log(TEXT("All training data cleared"));
}

FString UChessMLModule::PredictWithNeuralNetwork(const FString& BoardFen, const TArray<FString>& ValidMoves)
{
    if (NeuralNetworkWeights.Num() == 0 || ValidMoves.Num() == 0)
    {
        return TEXT("");
    }

    TArray<float> Features = ConvertBoardToFeatures(BoardFen);
    TArray<TArray<float>> LayerOutputs;
    TArray<float> Predictions = ForwardPass(Features, LayerOutputs);

    // 找到概率最高的合法走法
    float BestProbability = -1.0f;
    FString BestMove;

    for (const FString& Move : ValidMoves)
    {
        TArray<float> MoveFeatures = ConvertMoveToFeatures(Move);

        // 计算这个走法的概率
        float Probability = 0.0f;
        for (int32 i = 0; i < FMath::Min(Predictions.Num(), MoveFeatures.Num()); i++)
        {
            if (MoveFeatures[i] > 0.5f) // 这个位置有走法
            {
                Probability += Predictions[i];
                break; // 每个走法只对应一个输出位置
            }
        }

        if (Probability > BestProbability)
        {
            BestProbability = Probability;
            BestMove = Move;
        }
    }

    return BestMove;
}

FString UChessMLModule::PredictWithDecisionTree(const FString& BoardFen, const TArray<FString>& ValidMoves)
{
    if (!DecisionTree.IsValid() || ValidMoves.Num() == 0)
    {
        return TEXT("");
    }

    TArray<float> Features = ConvertBoardToFeatures(BoardFen);
    TArray<float> Predictions = DecisionTree->Predict(Features);

    // 找到概率最高的合法走法
    float BestScore = -MAX_FLT;
    FString BestMove;

    for (const FString& Move : ValidMoves)
    {
        TArray<float> MoveFeatures = ConvertMoveToFeatures(Move);

        // 计算这个走法的得分（简化实现）
        float Score = 0.0f;
        for (int32 i = 0; i < FMath::Min(Predictions.Num(), MoveFeatures.Num()); i++)
        {
            if (MoveFeatures[i] > 0.5f) // 这个位置有走法
            {
                Score += Predictions[i];
            }
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestMove = Move;
        }
    }

    return BestMove;
}

void UChessMLModule::LoadTrainingDataFromFile()
{
    FString DataPath = FPaths::ProjectSavedDir() + DataSavePath + TEXT("TrainingData.json");

    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *DataPath))
    {
        return; // 文件不存在是正常的
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);

    if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
    {
        TrainingData.Empty();

        const TArray<TSharedPtr<FJsonValue>>* DataArray;
        if (RootObject->TryGetArrayField(TEXT("TrainingData"), DataArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *DataArray)
            {
                TSharedPtr<FJsonObject> DataObject = Value->AsObject();
                if (DataObject.IsValid())
                {
                    FChessMLData Data;
                    DataObject->TryGetStringField(TEXT("BoardFen"), Data.BoardFen);
                    DataObject->TryGetStringField(TEXT("Move"), Data.Move);
                    DataObject->TryGetNumberField(TEXT("Score"), Data.Score);
                    DataObject->TryGetNumberField(TEXT("Depth"), Data.Depth);

                    FString TimestampString;
                    if (DataObject->TryGetStringField(TEXT("Timestamp"), TimestampString))
                    {
                        FDateTime::Parse(TimestampString, Data.Timestamp);
                    }

                    TrainingData.Add(Data);
                }
            }
        }
    }

    // 更新特征缓存
    UpdateFeatureCache();
}

void UChessMLModule::SaveTrainingDataToFile()
{
    FString DataPath = FPaths::ProjectSavedDir() + DataSavePath + TEXT("TrainingData.json");

    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> DataArray;

    for (const FChessMLData& Data : TrainingData)
    {
        TSharedPtr<FJsonObject> DataObject = MakeShared<FJsonObject>();
        DataObject->SetStringField(TEXT("BoardFen"), Data.BoardFen);
        DataObject->SetStringField(TEXT("Move"), Data.Move);
        DataObject->SetNumberField(TEXT("Score"), Data.Score);
        DataObject->SetNumberField(TEXT("Depth"), Data.Depth);
        DataObject->SetStringField(TEXT("Timestamp"), Data.Timestamp.ToString());

        DataArray.Add(MakeShared<FJsonValueObject>(DataObject));
    }

    RootObject->SetArrayField(TEXT("TrainingData"), DataArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(OutputString, *DataPath);
}

void UChessMLModule::UpdateFeatureCache()
{
    FeatureCache.Empty();
    LabelCache.Empty();

    for (const FChessMLData& Data : TrainingData)
    {
        TArray<float> Features = ConvertBoardToFeatures(Data.BoardFen);
        TArray<float> Labels = ConvertMoveToFeatures(Data.Move);

        if (Features.Num() > 0 && Labels.Num() > 0)
        {
            FeatureCache.Add(Features);
            LabelCache.Add(Labels);
        }
    }
}

void UChessMLModule::TrainNeuralNetwork(int32 Epochs)
{
    if (FeatureCache.Num() == 0 || LabelCache.Num() == 0)
    {
        ULogger::LogWarning(TEXT("No training data available"));
        return;
    }

    // 初始化神经网络权重（如果未初始化）
    if (NeuralNetworkWeights.Num() == 0)
    {
        InitializeNeuralNetwork();
    }

    for (int32 Epoch = 0; Epoch < Epochs; Epoch++)
    {
        float TotalLoss = 0.0f;
        int32 BatchCount = 0;

        // 小批量训练
        for (int32 BatchStart = 0; BatchStart < FeatureCache.Num(); BatchStart += TrainingBatchSize)
        {
            int32 BatchEnd = FMath::Min(BatchStart + TrainingBatchSize, FeatureCache.Num());

            // 前向传播
            TArray<TArray<float>> LayerOutputs;
            TArray<float> Output = ForwardPass(FeatureCache[BatchStart], LayerOutputs);

            // 计算损失
            float Loss = CalculateLoss(Output, LabelCache[BatchStart]);
            TotalLoss += Loss;

            // 反向传播
            BackwardPass(FeatureCache[BatchStart], LabelCache[BatchStart], LayerOutputs, Output);

            BatchCount++;
        }

        float AverageLoss = TotalLoss / BatchCount;
        TrainingResult.Loss = AverageLoss;

        if (Epoch % 10 == 0)
        {
            ULogger::Log(FString::Printf(TEXT("Epoch %d, Loss: %f"), Epoch, AverageLoss));
        }

        // 检查停止条件
        if (AverageLoss < 0.01f)
        {
            ULogger::Log(FString::Printf(TEXT("Training converged at epoch %d"), Epoch));
            break;
        }
    }
}

void UChessMLModule::TrainDecisionTree(int32 Epochs)
{
    if (!DecisionTree.IsValid())
    {
        DecisionTree = MakeShared<ChessDecisionTree>(10, 2);
    }

    if (FeatureCache.Num() > 0 && LabelCache.Num() > 0)
    {
        DecisionTree->Train(FeatureCache, LabelCache);
        TrainingResult.Accuracy = 0.8f; // 简化实现，实际应该计算准确率
    }
}

TArray<float> UChessMLModule::ForwardPass(const TArray<float>& Input, TArray<TArray<float>>& LayerOutputs)
{
    TArray<float> CurrentInput = Input;
    LayerOutputs.Empty();

    for (int32 Layer = 0; Layer < NeuralNetworkWeights.Num(); Layer++)
    {
        TArray<float> LayerOutput;
        LayerOutput.SetNum(NeuralNetworkBiases[Layer].Num());

        // 矩阵乘法: Output = Input * Weights + Biases
        for (int32 i = 0; i < NeuralNetworkWeights[Layer].Num(); i += CurrentInput.Num())
        {
            float Sum = 0.0f;
            for (int32 j = 0; j < CurrentInput.Num(); j++)
            {
                if (i + j < NeuralNetworkWeights[Layer].Num())
                {
                    Sum += CurrentInput[j] * NeuralNetworkWeights[Layer][i + j];
                }
            }
            int32 OutputIndex = i / CurrentInput.Num();
            if (OutputIndex < LayerOutput.Num())
            {
                LayerOutput[OutputIndex] = Sum + NeuralNetworkBiases[Layer][OutputIndex];
            }
        }

        // 激活函数
        if (Layer < NeuralNetworkWeights.Num() - 1)
        {
            ApplyReLU(LayerOutput); // 隐藏层用ReLU
        }
        else
        {
            ApplySoftmax(LayerOutput); // 输出层用Softmax
        }

        LayerOutputs.Add(LayerOutput);
        CurrentInput = LayerOutput;
    }

    return CurrentInput;
}

void UChessMLModule::BackwardPass(const TArray<float>& Input, const TArray<float>& Target,
    const TArray<TArray<float>>& LayerOutputs, const TArray<float>& Output)
{
    // 计算输出层误差
    TArray<float> Error = Output;
    for (int32 i = 0; i < Error.Num(); i++)
    {
        Error[i] -= Target[i];
    }

    // 反向传播误差
    for (int32 Layer = NeuralNetworkWeights.Num() - 1; Layer >= 0; Layer--)
    {
        TArray<float> Gradient = Error;

        // 计算梯度（如果是隐藏层，需要应用ReLU导数）
        if (Layer < NeuralNetworkWeights.Num() - 1)
        {
            for (int32 i = 0; i < Gradient.Num(); i++)
            {
                // ReLU导数：如果输出>0则为1，否则为0
                Gradient[i] *= (LayerOutputs[Layer][i] > 0.0f) ? 1.0f : 0.0f;
            }
        }

        // 更新权重
        TArray<float> PreviousOutput = (Layer == 0) ? Input : LayerOutputs[Layer - 1];

        for (int32 i = 0; i < NeuralNetworkWeights[Layer].Num(); i++)
        {
            int32 OutputIndex = i / PreviousOutput.Num();
            int32 InputIndex = i % PreviousOutput.Num();

            if (OutputIndex < Gradient.Num())
            {
                NeuralNetworkWeights[Layer][i] -= LearningRate * Gradient[OutputIndex] * PreviousOutput[InputIndex];
            }
        }

        // 更新偏置
        for (int32 i = 0; i < NeuralNetworkBiases[Layer].Num(); i++)
        {
            if (i < Gradient.Num())
            {
                NeuralNetworkBiases[Layer][i] -= LearningRate * Gradient[i];
            }
        }

        // 计算下一层的误差（如果需要）
        if (Layer > 0)
        {
            Error.SetNum(PreviousOutput.Num());
            for (int32 i = 0; i < PreviousOutput.Num(); i++)
            {
                Error[i] = 0.0f;
                for (int32 j = 0; j < Gradient.Num(); j++)
                {
                    int32 WeightIndex = j * PreviousOutput.Num() + i;
                    if (WeightIndex < NeuralNetworkWeights[Layer].Num())
                    {
                        Error[i] += Gradient[j] * NeuralNetworkWeights[Layer][WeightIndex];
                    }
                }
            }
        }
    }
}

void UChessMLModule::InitializeNeuralNetwork()
{
    // 简单的3层网络：输入层 -> 隐藏层 -> 输出层
    int32 InputSize = 10 * 9 * 16;     // 棋盘特征
    int32 HiddenSize = 256;           // 隐藏层大小
    int32 OutputSize = 10 * 9 * 10 * 9; // 所有可能走法

    NeuralNetworkWeights.SetNum(2);
    NeuralNetworkBiases.SetNum(2);

    // 初始化输入层到隐藏层
    NeuralNetworkWeights[0].SetNum(InputSize * HiddenSize);
    NeuralNetworkBiases[0].SetNum(HiddenSize);

    // 初始化隐藏层到输出层
    NeuralNetworkWeights[1].SetNum(HiddenSize * OutputSize);
    NeuralNetworkBiases[1].SetNum(OutputSize);

    // Xavier初始化权重
    float Scale1 = FMath::Sqrt(2.0f / (InputSize + HiddenSize));
    float Scale2 = FMath::Sqrt(2.0f / (HiddenSize + OutputSize));

    for (int32 i = 0; i < NeuralNetworkWeights[0].Num(); i++)
    {
        NeuralNetworkWeights[0][i] = FMath::FRandRange(-Scale1, Scale1);
    }

    for (int32 i = 0; i < NeuralNetworkWeights[1].Num(); i++)
    {
        NeuralNetworkWeights[1][i] = FMath::FRandRange(-Scale2, Scale2);
    }

    // 偏置初始化为0
    for (int32 i = 0; i < NeuralNetworkBiases[0].Num(); i++)
    {
        NeuralNetworkBiases[0][i] = 0.0f;
    }

    for (int32 i = 0; i < NeuralNetworkBiases[1].Num(); i++)
    {
        NeuralNetworkBiases[1][i] = 0.0f;
    }
}

TArray<float> UChessMLModule::ConvertBoardToFeatures(const FString& BoardFen)
{
    TArray<float> Features;
    Features.SetNum(10 * 9 * 16); // 10x9棋盘，16种棋子类型

    // 解析FEN字符串（简化版）
    // 实际象棋FEN格式：rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR
    TArray<FString> Rows;
    BoardFen.ParseIntoArray(Rows, TEXT("/"));

    int32 FeatureIndex = 0;

    for (int32 i = 0; i < Rows.Num(); i++)
    {
        FString Row = Rows[i];
        int32 ColIndex = 0;

        for (TCHAR Char : Row)
        {
            if (Char >= '1' && Char <= '9')
            {
                // 空格
                int32 EmptySpaces = Char - '0';
                for (int32 j = 0; j < EmptySpaces; j++)
                {
                    Features[FeatureIndex++] = 0.0f; // 空位
                }
                ColIndex += EmptySpaces;
            }
            else
            {
                // 棋子
                int32 PieceType = GetPieceTypeFromChar(Char);
                int32 PieceColor = FChar::IsUpper(Char) ? 0 : 1; // 红方0，黑方1

                // 设置特征
                int32 PieceIndex = PieceColor * 8 + PieceType;
                Features[FeatureIndex + PieceIndex] = 1.0f;
                FeatureIndex += 16; // 每个位置有16种可能

                ColIndex++;
            }
        }
    }

    return Features;
}

int32 UChessMLModule::GetPieceTypeFromChar(TCHAR Char)
{
    switch (FChar::ToLower(Char))
    {
    case 'k': return 0; // 将/帅
    case 'a': return 1; // 士/仕
    case 'b': return 2; // 象/相
    case 'n': return 3; // 马
    case 'r': return 4; // 车
    case 'c': return 5; // 炮
    case 'p': return 6; // 兵/卒
    default: return 7;  // 未知
    }
}

TArray<float> UChessMLModule::ConvertMoveToFeatures(const FString& Move)
{
    TArray<float> Features;
    Features.SetNum(10 * 9 * 10 * 9); // 所有可能的走法

    if (Move.Len() < 4) return Features;

    // 解析走法格式："a0a1" 或 "h2e2"
    int32 FromX = Move[0] - 'a';
    int32 FromY = Move[1] - '0';
    int32 ToX = Move[2] - 'a';
    int32 ToY = Move[3] - '0';

    if (FromX >= 0 && FromX < 9 && FromY >= 0 && FromY < 10 &&
        ToX >= 0 && ToX < 9 && ToY >= 0 && ToY < 10)
    {
        int32 MoveIndex = FromY * 9 * 10 * 9 + FromX * 10 * 9 + ToY * 9 + ToX;
        if (MoveIndex >= 0 && MoveIndex < Features.Num())
        {
            Features[MoveIndex] = 1.0f;
        }
    }

    return Features;
}

float UChessMLModule::CalculateLoss(const TArray<float>& Output, const TArray<float>& Target)
{
    // 交叉熵损失
    float Loss = 0.0f;
    for (int32 i = 0; i < Output.Num(); i++)
    {
        if (Target[i] > 0.5f) // 这是正确的走法
        {
            Loss += -FMath::Loge(FMath::Max(Output[i], 1e-8f));
        }
    }
    return Loss;
}

void UChessMLModule::ApplyReLU(TArray<float>& Data)
{
    for (float& Value : Data)
    {
        Value = FMath::Max(0.0f, Value);
    }
}

void UChessMLModule::ApplySoftmax(TArray<float>& Data)
{
    float MaxVal = TNumericLimits<float>::Lowest();
    for (float Val : Data)
    {
        if (Val > MaxVal) MaxVal = Val;
    }

    float Sum = 0.0f;
    for (float& Val : Data)
    {
        Val = FMath::Exp(Val - MaxVal); // 数值稳定性
        Sum += Val;
    }

    for (float& Val : Data)
    {
        Val /= Sum;
    }
}