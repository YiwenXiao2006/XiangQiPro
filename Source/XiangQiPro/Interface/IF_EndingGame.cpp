// Copyright 2026 Ultimate Player All Rights Reserved.


#include "IF_EndingGame.h"

// Add default functionality here for any IIF_EndingGame functions that are not pure virtual.

void IIF_EndingGame::OnEndingGameStart(UObject* OwnerObject, int32 Index)
{
	if (OwnerObject)
	{
		Execute_OnEndingGameStart(OwnerObject, Index); // Call the function in your blueprint.
	}
}

void IIF_EndingGame::OnJueSha(UObject* OwnerObject)
{
	if (OwnerObject)
	{
		Execute_OnJueSha(OwnerObject); // Call the function in your blueprint.
	}
}
