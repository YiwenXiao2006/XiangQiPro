// Copyright 2026 Ultimate Player All Rights Reserved.


#include "AsyncWorker.h"
#include "Engine/Engine.h"
#include "Logger.h"

UAsyncWorker::UAsyncWorker()
    : BackgroundTask(nullptr)
    , bShouldStop(false)
    , CurrentState(EAsyncWorkerState::Idle)
{
    // 防止被垃圾回收
    AddToRoot();
}

UAsyncWorker::~UAsyncWorker()
{
    StopAsyncWork();
}

UAsyncWorker* UAsyncWorker::CreateAndStartWorker(TFunction<void(std::atomic<bool>&)> WorkFunction, TFunction<void(EAsyncWorkerState)> CompletedCallback, TFunction<void(float)> ProgressCallback)
{
    UAsyncWorker* Worker = NewObject<UAsyncWorker>();

    // 设置工作函数
    Worker->SetWorkFunction(WorkFunction);

    // 设置完成回调
    FOnAsyncWorkCompleted CompletedDelegate;
    CompletedDelegate.BindLambda([CompletedCallback, Worker](EAsyncWorkerState State)
        {
            if (CompletedCallback)
            {
                CompletedCallback(State);
            }
        });

    // 设置进度回调
    if (ProgressCallback)
    {
        FOnAsyncWorkProgress ProgressDelegate;
        ProgressDelegate.BindLambda([ProgressCallback](float Progress)
            {
                ProgressCallback(Progress);
            });
        Worker->StartAsyncWorkWithProgress(CompletedDelegate, ProgressDelegate);
    }
    else
    {
        Worker->StartAsyncWork(CompletedDelegate);
    }

    return Worker;
}

bool UAsyncWorker::StartAsyncWork(const FOnAsyncWorkCompleted& InCompletedDelegate)
{
    CompletedDelegate = InCompletedDelegate;
    return StartWorkInternal();
}

bool UAsyncWorker::StartAsyncWorkWithProgress(
    const FOnAsyncWorkCompleted& InCompletedDelegate,
    const FOnAsyncWorkProgress& InProgressDelegate)
{
    CompletedDelegate = InCompletedDelegate;
    ProgressDelegate = InProgressDelegate;
    return StartWorkInternal();
}

bool UAsyncWorker::StartWorkInternal()
{
    FScopeLock Lock(&CriticalSection);

    if (CurrentState == EAsyncWorkerState::Running)
    {
        ULogger::LogWarning(TEXT("UAsyncWorker::StartWorkInternal: 异步工作器已在运行中"));
        return false;
    }

    // 检查是否有工作函数
    if (!WorkFunction)
    {
        ULogger::LogError(TEXT("UAsyncWorker::StartWorkInternal: 没有设置工作函数，请先调用SetWorkFunction"));
        return false;
    }

    // 重置状态
    bShouldStop = false;
    SetState(EAsyncWorkerState::Running);

    // 创建并启动异步任务
    BackgroundTask = new FAsyncTask<FWorkerTask>(this);
    BackgroundTask->StartBackgroundTask();

    ULogger::Log(TEXT("UAsyncWorker::StartWorkInternal: 异步工作器启动成功"));
    return true;
}

void UAsyncWorker::StopAsyncWork()
{
    FScopeLock Lock(&CriticalSection);

    if (CurrentState != EAsyncWorkerState::Running)
    {
        return;
    }

    // 设置停止标志
    bShouldStop = true;
    SetState(EAsyncWorkerState::Cancelled);

    // 等待任务完成
    if (BackgroundTask)
    {
        BackgroundTask->EnsureCompletion(false, true);
        BackgroundTask = nullptr;
    }

    ULogger::Log(TEXT("UAsyncWorker::StopAsyncWork: 异步工作器已停止"));
}

void UAsyncWorker::ReportProgress(float Progress)
{
    // 将进度更新派送到主线程
    if (IsInGameThread())
    {
        OnProgressUpdated(Progress);
    }
    else
    {
        // 使用 FFunctionGraphTask 替代 AsyncTask 宏
        FFunctionGraphTask::CreateAndDispatchWhenReady([this, Progress]()
            {
                OnProgressUpdated(Progress);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
    }
}

void UAsyncWorker::BeginDestroy()
{
    StopAsyncWork();
    RemoveFromRoot();
    Super::BeginDestroy();
}

void UAsyncWorker::SetState(EAsyncWorkerState NewState)
{
    FScopeLock Lock(&CriticalSection);
    CurrentState = NewState;
}

void UAsyncWorker::OnWorkCompleted()
{
    FScopeLock Lock(&CriticalSection);

    // 执行完成委托
    if (CompletedDelegate.IsBound())
    {
        CompletedDelegate.Execute(CurrentState);
    }

    // 允许垃圾回收
    RemoveFromRoot();

    // 清理异步任务
    if (BackgroundTask)
    {
        //delete BackgroundTask;
        BackgroundTask = nullptr;
    }

    ULogger::Log(TEXT("UAsyncWorker::OnWorkCompleted: 异步工作器完成"));
}

void UAsyncWorker::OnProgressUpdated(float Progress)
{
    if (ProgressDelegate.IsBound())
    {
        ProgressDelegate.Execute(FMath::Clamp(Progress, 0.0f, 1.0f));
    }
}

void UAsyncWorker::ExecuteWorkFunction()
{
    if (WorkFunction)
    {
        WorkFunction(bShouldStop);
    }
}

// 异步任务实现
void UAsyncWorker::FWorkerTask::DoWork()
{
    if (!Worker || Worker->bShouldStop)
    {
        return;
    }

    try
    {
        // 执行工作函数
        Worker->ExecuteWorkFunction();

        // 如果没有被取消，标记为完成
        if (!Worker->bShouldStop)
        {
            Worker->SetState(EAsyncWorkerState::Completed);
        }

        // 在主线程执行完成回调
        FFunctionGraphTask::CreateAndDispatchWhenReady([WorkerPtr = TWeakObjectPtr<UAsyncWorker>(Worker)]()
            {
                if (WorkerPtr.IsValid())
                {
                    WorkerPtr->OnWorkCompleted();
                }
            }, TStatId(), nullptr, ENamedThreads::GameThread);
    }
    catch (const std::exception& e)
    {
        ULogger::LogError(FString::Printf(TEXT("UAsyncWorker::FWorkerTask::DoWork: 异步工作器异常: %s"), UTF8_TO_TCHAR(e.what())));

        // 异常情况下也执行完成回调
        FFunctionGraphTask::CreateAndDispatchWhenReady([WorkerPtr = TWeakObjectPtr<UAsyncWorker>(Worker)]()
            {
                if (WorkerPtr.IsValid())
                {
                    WorkerPtr->SetState(EAsyncWorkerState::Cancelled);
                    WorkerPtr->OnWorkCompleted();
                }
            }, TStatId(), nullptr, ENamedThreads::GameThread);
    }
}
