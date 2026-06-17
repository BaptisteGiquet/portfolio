#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"

namespace EMREndSessionTrace
{
	static const TCHAR* TravelOptionKey = TEXT("EndSessionTrace");

	inline bool IsEnabledForWorld(const UWorld* World)
	{
		if (!World)
		{
			return false;
		}

		return World->URL.HasOption(TravelOptionKey);
	}

	inline bool IsEnabled(const UObject* ContextObject)
	{
		if (!ContextObject)
		{
			return false;
		}

		if (const UWorld* AsWorld = Cast<UWorld>(ContextObject))
		{
			return IsEnabledForWorld(AsWorld);
		}

		return IsEnabledForWorld(ContextObject->GetWorld());
	}
}

#define EMR_END_SESSION_TRACE(ContextObject, Format, ...) \
	do \
	{ \
	if (EMREndSessionTrace::IsEnabled(ContextObject)) \
		{ \
			UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__); \
	} \
	} while (false)
