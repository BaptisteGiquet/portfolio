#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

namespace Debug
{
	static void Print(const FString& InMessage, int32 InKey = -1, const FColor& InColor = FColor::MakeRandomColor())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 7.f, InColor, InMessage);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *InMessage);
		}
	}
}
