// Copyright 2026 Ultimate Player All Rights Reserved.

#include "AsyncWorker.h"
#include "Engine/Engine.h"
#include "Logger.h"
#include "HAL/Event.h"

UAsyncWorker::UAsyncWorker()
    : BackgroundTask(nullptr)
    , bShouldStop(false)
    , bShouldPause(false)
    , CurrentState(EAsyncWorkerState::Idle)
    , PauseEvent(FPlatformProcess::GetSynchEventFromPool(false))
{
    // 防止被垃圾回收
    AddToRoot();
}

UAsyncWorker::~UAsyncWorker()
{
    StopAsyncWork();

    // 清理事件
    if (PauseEvent)
    {
        FPlatformProcess::ReturnSynchEventToPool(PauseEvent);
        PauseEvent = nullptr;
    }
}

UAsyncWorker* UAsyncWorker::CreateAndStartWorker(TFunction<void(UAsyncWorker*)> WorkFunction, TFunction<void(EAsyncWorkerState)> CompletedCallback, TFunction<void(float)> ProgressCallback)
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

    if (CurrentState == EAsyncWorkerState::Running || CurrentState == EAsyncWorkerState::Paused)
    {
        ULogger::LogWarning(TEXT("UAsyncWorker::StartWorkInternal: 异步工作器已在运行或暂停中"));
        return false;
    }

    // 检查是否有工作函数
    if (!WorkFunction)
    {
        ULogger::LogError(TEXT("UAsyncWorker::StartAsyncWork: 没有设置工作函数，请先调用SetWorkFunction"));
        return false;
    }

    // 重置状态
    bShouldStop = false;
    bShouldPause = false;
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

    if (CurrentState != EAsyncWorkerState::Running && CurrentState != EAsyncWorkerState::Paused)
    {
        return;
    }

    // 设置停止标志
    bShouldStop = true;

    // 如果处于暂停状态，需要触发事件让线程继续以便检测停止
    if (CurrentState == EAsyncWorkerState::Paused && PauseEvent)
    {
        PauseEvent->Trigger();
    }

    SetState(EAsyncWorkerState::Cancelled);

    // 等待任务完成
    if (BackgroundTask)
    {
        BackgroundTask->EnsureCompletion(false, true);
        BackgroundTask = nullptr;
    }

    ULogger::Log(TEXT("UAsyncWorker::StopAsyncWork: 异步工作器已停止"));
}

bool UAsyncWorker::PauseAsyncWork()
{
    FScopeLock Lock(&CriticalSection);

    if (CurrentState != EAsyncWorkerState::Running)
    {
        ULogger::LogWarning(TEXT("UAsyncWorker::PauseAsyncWork: 只有运行中的工作器可以暂停"));
        return false;
    }

    bShouldPause = true;
    SetState(EAsyncWorkerState::Paused);

    ULogger::Log(TEXT("UAsyncWorker::PauseAsyncWork: 异步工作器已暂停"));
    return true;
}

bool UAsyncWorker::ResumeAsyncWork()
{
    FScopeLock Lock(&CriticalSection);

    if (CurrentState != EAsyncWorkerState::Paused)
    {
        ULogger::LogWarning(TEXT("UAsyncWorker::ResumeAsyncWork: 只有暂停中的工作器可以恢复"));
        return false;
    }

    bShouldPause = false;
    SetState(EAsyncWorkerState::Running);

    // 触发事件唤醒工作线程
    if (PauseEvent)
    {
        PauseEvent->Trigger();
    }

    ULogger::Log(TEXT("UAsyncWorker::ResumeAsyncWork: 异步工作器已恢复"));
    return true;
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

void UAsyncWorker::CheckPause()
{
    // 如果应该暂停，则进入等待状态
    if (bShouldPause && !bShouldStop)
    {
        WaitForResume();
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
        WorkFunction(this);
    }
}

void UAsyncWorker::WaitForResume()
{
    if (PauseEvent)
    {
        // 等待恢复事件，超时时间设为100毫秒以便定期检查停止标志
        while (bShouldPause && !bShouldStop)
        {
            PauseEvent->Wait(100); // 等待100毫秒
        }
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