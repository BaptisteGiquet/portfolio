#pragma once

#include "EMRGASConfig.generated.h"

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None			UMETA(DisplayName = "None"),
	BasicAttack		UMETA(DisplayName = "Basic Attack"),
	Aim				UMETA(DisplayName = "Aim"),
	AbilityOne		UMETA(DisplayName = "Ability One"),
	AbilityTwo		UMETA(DisplayName = "Ability Two"),
	AbilityThree	UMETA(DisplayName = "Ability Three"),
	AbilityFour		UMETA(DisplayName = "Ability Four"),
	AbilityFive		UMETA(DisplayName = "Ability Five"),
	AbilitySix		UMETA(DisplayName = "Ability Six"),
	Confirm		UMETA(DisplayName = "Confirm"),
	Cancel		UMETA(DisplayName = "Cancel"),
};
