#pragma once

#include "CoreMinimal.h"

LOD_API FName GetUIStringTableId();

#define UIStringTableId GetUIStringTableId()

static FText GetLocalizedText(const FName& StringTableId, const FString& Key)
{
	return FText::FromStringTable(StringTableId, Key);
}
