#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AGASSInteractableInterface.generated.h"

class AController;

UINTERFACE(Blueprintable)
class AGASSCOMMON_API UAGASSInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class AGASSCOMMON_API IAGASSInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Interaction")
	bool CanAGASSInteract(AController* InteractingController) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Interaction")
	float GetAGASSInteractHoldDurationSeconds(AController* InteractingController) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Interaction")
	void HandleAGASSInteract(AController* InteractingController);
};
