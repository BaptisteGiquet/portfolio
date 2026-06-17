#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS/EMRGASConfig.h"
#include "LOD/EMRCollisionChannels.h"
#include "EMRPlayerWidgetInteractionComponent.generated.h"

class UCameraComponent;
class UWidgetInteractionComponent;
struct FHitResult;

UCLASS(ClassGroup = (EMR), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRPlayerWidgetInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRPlayerWidgetInteractionComponent();

    bool TryHandleWorldWidgetClick(
        EAbilityInputID AbilityInputID,
        UCameraComponent* Camera,
        UWidgetInteractionComponent* WorldWidgetInteraction,
        ECollisionChannel TraceChannel,
        float TraceDistance) const;

    bool HasWorldWidgetUnderCrosshair(
        FHitResult& OutHit,
        const UCameraComponent* Camera,
        const UWidgetInteractionComponent* WorldWidgetInteraction,
        ECollisionChannel TraceChannel,
        float TraceDistance) const;
};

