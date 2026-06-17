#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "AGASSLocalPlayer.generated.h"

class UAGASSSettingsLocal;

UCLASS()
class AGASSCOMMON_API UAGASSLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	UFUNCTION()
	UAGASSSettingsLocal* GetLocalSettings() const;
};
