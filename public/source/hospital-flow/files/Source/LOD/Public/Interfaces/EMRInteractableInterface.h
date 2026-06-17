#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UObject/Interface.h"
#include "EMRInteractableInterface.generated.h"

UINTERFACE()
class UEMRInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Interaction")
	FGameplayEventData GetInteractionEventData(AActor* InterActor) const;

	/** User-facing display name for UI prompts. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Interaction")
	FText GetInteractableDisplayName() const;

	/** Whether the interactable can currently be interacted with for client-side filtering/highlighting. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Interaction")
	bool IsInteractableEnabled() const;
};
