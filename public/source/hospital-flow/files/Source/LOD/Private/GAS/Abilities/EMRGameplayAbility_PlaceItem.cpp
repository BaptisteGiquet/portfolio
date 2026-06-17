
#include "GAS/Abilities/EMRGameplayAbility_PlaceItem.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "GAS/EMRTags.h"
#include "GameFramework/Character.h"
#include "Shop/EMRItemActor.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRItemPlacementComponent.h"


UEMRGameplayAbility_PlaceItem::UEMRGameplayAbility_PlaceItem()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = EMRTags::Event::Item::Place;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}


bool UEMRGameplayAbility_PlaceItem::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
   
	Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	AEMRItemActor* CarriedItem = nullptr;
	return TryGetCarriedItem(ActorInfo, CarriedItem);
}


void UEMRGameplayAbility_PlaceItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateAbility: CommitAbility failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (!Character)
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivateAbility: No avatar character."));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		AEMRItemActor* CarriedItem = nullptr;
		if (TriggerEventData)
		{
			CarriedItem = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->Target));
		}

		if (!CarriedItem)
		{
			TryGetCarriedItem(ActorInfo, CarriedItem);
		}
		
		if (!CarriedItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivateAbility: No carried clinic item found, aborting."));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		if (UEMRFixedPlacementComponent* FixedPlacement = CarriedItem->FindComponentByClass<UEMRFixedPlacementComponent>())
		{
			if (FixedPlacement->HasAnchor())
			{
				if (HasAuthority(&ActivationInfo))
				{
					const FTransform AnchorTransform = FixedPlacement->GetAnchorTransform();
					CarriedItem->DropAtLocation(AnchorTransform.GetLocation());
					CarriedItem->SetActorRotation(AnchorTransform.GetRotation());

					if (UEMRCarryableComponent* Carryable = CarriedItem->GetCarryableComponent())
					{
						Carryable->SetLockedInPlace(true);
					}
				}

				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
				return;
			}
		}

		
		UEMRItemPlacementComponent* PlacementComponent = Character->FindComponentByClass<UEMRItemPlacementComponent>();
		if (!PlacementComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivateAbility: No placement component found."));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		FEMRItemPlacementResult PlacementResult;
		bool bPlacementQuerySucceeded = false;

		if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
		{
			const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Get(0);
			if (TargetData && TargetData->GetScriptStruct() == FGameplayAbilityTargetData_SingleTargetHit::StaticStruct())
			{
				const FGameplayAbilityTargetData_SingleTargetHit* HitData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(TargetData);
				bPlacementQuerySucceeded = PlacementComponent->BuildPlacementFromHit(*Character, *CarriedItem, HitData->HitResult, PlacementResult);
			}
		}

		if (!bPlacementQuerySucceeded)
		{
			FVector ViewLocation = Character->GetPawnViewLocation();
			FRotator ViewRotation = Character->GetViewRotation();
			if (AController* Controller = Character->GetController())
			{
				Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
			}

			bPlacementQuerySucceeded = PlacementComponent->QueryPlacement(*Character, *CarriedItem, ViewLocation, ViewRotation.Vector(), PlacementResult);
		}

		if (!bPlacementQuerySucceeded || !PlacementResult.bHasHit || !PlacementResult.bIsValid)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}


		if (HasAuthority(&ActivationInfo))
		{
			CarriedItem->DropAtLocation(PlacementResult.Transform.GetLocation());
			CarriedItem->SetActorRotation(PlacementResult.Transform.GetRotation());
		}
	}
	
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


bool UEMRGameplayAbility_PlaceItem::TryGetCarriedItem(const FGameplayAbilityActorInfo* ActorInfo, AEMRItemActor*& OutItem) const
{
    OutItem = nullptr;
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryGetCarriedItem: No avatar character."));
        return false;
    }

    TArray<AActor*> AttachedActors;
    Character->GetAttachedActors(AttachedActors);

    for (AActor* AttachedActor : AttachedActors)
    {
        AEMRItemActor* Item = Cast<AEMRItemActor>(AttachedActor);
        if (!Item)
        {
            continue;
        }

        if (UEMRCarryableComponent* Carryable = Item->FindComponentByClass<UEMRCarryableComponent>())
        {
            if (Carryable->IsCarried() && Item->GetAttachParentActor() == Character)
            {
                OutItem = Item;
                return true;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("TryGetCarriedItem: No carried clinic item found."));
    return false;
}


