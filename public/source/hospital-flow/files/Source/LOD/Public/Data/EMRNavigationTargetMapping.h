#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRNavigationTargetMapping.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FEMRNavigationTargetData : public FTableRowBase
{
    GENERATED_BODY()

    /** Tag that triggers a navigation intent when applied to a patient. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "GameplayTag"))
    FGameplayTag TriggerTag;

    /** Message tag to broadcast through the Gameplay Message Subsystem. Falls back to TriggerTag when unset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "GameplayTag"))
    FGameplayTag MessageTag;

    /** Optional machine type tag to resolve the navigation target dynamically. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "GameplayTag"))
    FGameplayTag MachineTypeTag;

    /** Explicit navigation target actor to use when no machine type lookup is desired. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<AActor> TargetActor;
};
