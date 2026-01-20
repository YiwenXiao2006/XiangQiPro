// Copyright 2026 Ultimate Player All Rights Reserved.


#include "IF_GameState.h"

void IIF_GameState::GamePause(UObject* OwnerObject)
{
	if (OwnerObject)
	{
		Execute_GamePause(OwnerObject); // Call the function in your blueprint.
	}
}

void IIF_GameState::GameResume(UObject* OwnerObject)
{
	if (OwnerObject)
	{
		Execute_GameResume(OwnerObject); // Call the function in your blueprint.
	}
}

void IIF_GameState::GameOver(UObject* OwnerObject)
{
	if (OwnerObject)
	{
		Execute_GameOver(OwnerObject); // Call the function in your blueprint.
	}
}

void IIF_GameState::GamePlayAgain(UObject* OwnerObject)
{
	if (OwnerObject)
	{
		Execute_GamePlayAgain(OwnerObject); // Call the function in your blueprint.
	}
}
