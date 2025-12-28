// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Async/AsyncWork.h"
#include "AsyncWorker.generated.h"

// 异步操作状态
UENUM(BlueprintType)
enum class EAsyncWorkerState : uint8
{
    Idle,       // 空闲
    Running,    // 运行中
    Completed,  // 完成
    Cancelled   // 取消
};

DECLARE_DELEGATE_OneParam(FOnAsyncWorkCompleted, EAsyncWorkerState);
DECLARE_DELEGATE_OneParam(FOnAsyncWorkProgress, float);
DECLARE_DELEGATE_OneParam(FOnWorkDelegate, TAtomic<bool>&);

/**
 * 异步工作器，封装耗时操作的线程管理
 */
UCLASS()
class XIANGQIPRO_API UAsyncWorker : public UObject
{
	GENERATED_BODY()

public:
    UAsyncWorker();
    virtual ~UAsyncWorker();

    // 创建异步工作器
    static UAsyncWorker* CreateAndStartWorker(
        TFunction<void(std::atomic<bool>&)> WorkFunction,
        TFunction<void(EAsyncWorkerState)> CompletedCallback = nullptr,
        TFunction<void(float)> ProgressCallback = nullptr);

    /**
     * 开始异步工作（基础版本，不带进度委托）
     */
    bool StartAsyncWork(
        const FOnAsyncWorkCompleted& CompletedDelegate);

    /**
     * 开始异步工作（带进度委托版本）
     */
    bool StartAsyncWorkWithProgress(
        const FOnAsyncWorkCompleted& CompletedDelegate,
        const FOnAsyncWorkProgress& ProgressDelegate);

    /**
     * 停止异步工作（线程安全）
     */
    UFUNCTION(BlueprintCallable, Category = "Async")
    void StopAsyncWork();

    /**
     * 获取当前状态
     */
    UFUNCTION(BlueprintCallable, Category = "Async")
    EAsyncWorkerState GetCurrentState() const { return CurrentState; }

    /**
     * 是否正在运行
     */
    UFUNCTION(BlueprintCallable, Category = "Async")
    bool IsRunning() const { return CurrentState == EAsyncWorkerState::Running; }

    /**
     * 设置工作函数
     */
    void SetWorkFunction(TFunction<void(std::atomic<bool>&)> InWorkFunction) { WorkFunction = MoveTemp(InWorkFunction); }

    /**
     * 更新进度（线程安全，可在工作函数中调用）
     */
    void ReportProgress(float Progress);

protected:
    virtual void BeginDestroy() override;

private:
    // 内部异步任务类
    class FWorkerTask : public FNonAbandonableTask
    {
    public:
        FWorkerTask(UAsyncWorker* InWorker)
            : Worker(InWorker) {
        }

        void DoWork();
        FORCEINLINE TStatId GetStatId() const
        {
            RETURN_QUICK_DECLARE_CYCLE_STAT(FWorkerTask, STATGROUP_ThreadPoolAsyncTasks);
        }

    private:
        UAsyncWorker* Worker;
    };

    // 实际启动工作的方法
    bool StartWorkInternal();

    // 线程安全的状态更新
    void SetState(EAsyncWorkerState NewState);

    // 完成回调（在主线程执行）
    void OnWorkCompleted();

    // 进度更新回调（在主线程执行）
    void OnProgressUpdated(float Progress);

    // 在工作线程中执行工作函数
    void ExecuteWorkFunction();

private:
    // 异步任务实例（避免使用AsyncTask名称）
    FAsyncTask<FWorkerTask>* BackgroundTask;

    // 线程安全的取消标志
    std::atomic<bool> bShouldStop;

    // 当前状态
    std::atomic<EAsyncWorkerState> CurrentState;

    // 工作函数
    TFunction<void(std::atomic<bool>&)> WorkFunction;

    // 完成委托
    FOnAsyncWorkCompleted CompletedDelegate;

    // 进度委托
    FOnAsyncWorkProgress ProgressDelegate;

    // 线程同步
    FCriticalSection CriticalSection;

};
