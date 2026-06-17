
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaAbilityInterface.generated.h"


class UMobaAbilitySystemGenerics;
class UGameplayAbility;

UINTERFACE()
class UMobaAbilityInterface : public UInterface
{
	GENERATED_BODY()
};


class LOD_API IMobaAbilityInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& GetMainAbilities() const = 0;

	virtual const UMobaAbilitySystemGenerics* GetAbilitySystemGenerics() const = 0;
};
