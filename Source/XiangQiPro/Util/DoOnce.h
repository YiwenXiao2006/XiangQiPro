// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

template <typename FunctionType>
struct FDoOnce;

/*
* 设置一个仅执行一次的代码块
*/
template <typename Ret, typename... ParamTypes>
struct FDoOnce<Ret(ParamTypes...)>
{

public:

	FORCEINLINE FDoOnce() : bDoOnce(true), WorkFunction(nullptr) {}

	FORCEINLINE FDoOnce(TFunction<Ret(ParamTypes...)> Function) : bDoOnce(true), WorkFunction(Function) {}

	FORCEINLINE void SetWorkFunction(TFunction<Ret(ParamTypes...)> Function)
	{
		WorkFunction = Function;
	}

	FORCEINLINE void Reset() 
	{ 
		bDoOnce = true; 
		WorkFunction = nullptr;
	}

	FORCEINLINE bool IsExecutable() const
	{ 
		return bDoOnce; 
	}

	FORCEINLINE Ret Execute(ParamTypes... Params)
	{
		if (bDoOnce && WorkFunction)
		{
			bDoOnce = false;
			Ret value = WorkFunction(Params...); // 执行操作
			WorkFunction = nullptr;
			return value; // 返回
		}
		return Ret();
	}

private:

	bool bDoOnce;

	TFunction<Ret(ParamTypes...)> WorkFunction;
};

/*
* 设置一个仅执行一次的代码块, 且无返回值
*/
template <typename... ParamTypes>
struct FDoOnce<void(ParamTypes...)>
{
public:
	FORCEINLINE FDoOnce() : bDoOnce(true), WorkFunction(nullptr) {}

	FORCEINLINE FDoOnce(TFunction<void(ParamTypes...)> Function) : bDoOnce(true), WorkFunction(Function) {}

	FORCEINLINE void SetWorkFunction(TFunction<void(ParamTypes...)> Function)
	{
		WorkFunction = Function;
	}

	FORCEINLINE void Reset()
	{
		bDoOnce = true;
		WorkFunction = nullptr;
	}

	FORCEINLINE bool IsExecutable() const
	{
		return bDoOnce;
	}

	FORCEINLINE void Execute(ParamTypes... Params)
	{
		if (bDoOnce && WorkFunction)
		{
			bDoOnce = false;
			WorkFunction(Params...);
			WorkFunction = nullptr;
		}
	}

private:
	bool bDoOnce;
	TFunction<void(ParamTypes...)> WorkFunction;
};