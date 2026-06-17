#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "EMRSeatedAnimationInterface.generated.h"

UINTERFACE(BlueprintType)
class UEMRSeatedAnimationInterface : public UInterface
{
    GENERATED_BODY()
};

class LOD_API IEMRSeatedAnimationInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Seating")
    void SetSeatedAnimationTag(const FGameplayTag& InTag);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Seating")
    FGameplayTag GetSeatedAnimationTag() const;
};
