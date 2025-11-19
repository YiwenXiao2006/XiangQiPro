// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "Logger.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectManager.generated.h"

typedef UObjectManager OM;

/**
 * A class for loading assets from local data
 */
UCLASS()
class XIANGQIPRO_API UObjectManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	template <typename T>
	static T* GetObject(const TCHAR* ObjectToFind, UObject* Outer = nullptr)
	{
		return LoadObject<T>(Outer, ObjectToFind);
	}

	template <typename T>
	static UClass* GetBlueprint(const TCHAR* ClassToFind, UObject* Outer = nullptr)
	{
		return LoadClass<T>(Outer, ClassToFind);
	}

	template <typename T>
	static T* GetConstructorObject(const TCHAR* ObjectToFind, bool bPrintLog = false)
	{
		ConstructorHelpers::FObjectFinder<T> Obj(ObjectToFind);
		if (bPrintLog)
		{
			if (!Obj.Succeeded())
			{
				ULogger::LogWarning(TEXT("FClassFinder"), FString("Fail to find: ").Append(ObjectToFind));
			}
		}
		return Obj.Object;
	}

	template <typename T>
	static TSubclassOf<T> GetConstructorBlueprint(const TCHAR* ClassToFind, bool bPrintLog = false)
	{
		ConstructorHelpers::FClassFinder<T> BP(ClassToFind);
		if (bPrintLog)
		{
			if (!BP.Succeeded())
			{
				ULogger::LogWarning(TEXT("FClassFinder"), FString("Fail to find: ").Append(ClassToFind));
			}
		}
		return BP.Class;
	}
};
