#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "EMRAbilitySystemComponent.generated.h"

enum class EAbilityInputID : uint8;
class UEMRAbilitySystemGenerics;

UCLASS()
class UEMRAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UEMRAbilitySystemComponent();
	
};