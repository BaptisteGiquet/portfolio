#include "GAS/Abilities/EMRGameplayAbility_PickItem.h"

#include "GAS/EMRTags.h"
#include "GameFramework/Character.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Shop/EMRItemActor.h"


UEMRGameplayAbility_PickItem::UEMRGameplayAbility_PickItem()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag   = EMRTags::Event::Item::Pick;
	TriggerData.TriggerSource  = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_PickItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	ACharacter* Character = GetAvatarCharacter();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* TargetActor = const_cast<AActor*>(Cast<AActor>(TriggerEventData->Target));
	if (!TargetActor && TriggerEventData)
	{
		TargetActor = const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject));
	}
	
	if (!TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* CurrentlyCarried = FindCarriedActor(*Character);
	if (AEMRItemActor* Item = Cast<AEMRItemActor>(TargetActor))
	{
		HandleClinicItem(*Item, *Character, CurrentlyCarried);
	}
	else
	{
	}


	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


void UEMRGameplayAbility_PickItem::HandleClinicItem(AEMRItemActor& Item, ACharacter& Character, AActor* CurrentlyCarried) const
{
	UEMRCarryableComponent* ItemCarryableComp = Item.GetCarryableComponent();
	if (!ItemCarryableComp)
	{
		return;
	}

	
	if (ItemCarryableComp->IsCarried())
	{
		if (Item.GetAttachParentActor() == &Character)
		{
			if (UEMRFixedPlacementComponent* FixedPlacement = Item.FindComponentByClass<UEMRFixedPlacementComponent>())
			{
				if (FixedPlacement->HasAnchor())
				{
					const FTransform AnchorTransform = FixedPlacement->GetAnchorTransform();
					Item.DropAtLocation(AnchorTransform.GetLocation());
					Item.SetActorRotation(AnchorTransform.GetRotation());
					ItemCarryableComp->SetLockedInPlace(true);
					return;
				}
			}

			Item.DropAtLocation(ComputeDropLocation(Character));
		}
		else
		{
		}
	}
	else
	{
		if (UEMRFixedPlacementComponent* FixedPlacement = Item.FindComponentByClass<UEMRFixedPlacementComponent>())
		{
			if (FixedPlacement->HasAnchor())
			{
				ItemCarryableComp->SetLockedInPlace(false);
			}
		}

		const FName CarrySocketName = Item.GetHandSocketName();
		if (CarrySocketName.IsNone())
		{
			return;
		}

		if (USkeletalMeshComponent* HandMesh = UEMRCarryableComponent::FindCarrierMesh(Character, CarrySocketName))
		{
			Item.Pickup(HandMesh);
		}
		else
		{
		}
	}
}


AActor* UEMRGameplayAbility_PickItem::FindCarriedActor(const ACharacter& Character) const
{
	TArray<AActor*> AttachedActors;
	Character.GetAttachedActors(AttachedActors);

	for (AActor* Attached : AttachedActors)
	{
		if (!Attached)
		{
			continue;
		}

		if (UEMRCarryableComponent* Carryable = Attached->FindComponentByClass<UEMRCarryableComponent>())
		{
			if (Carryable->IsCarried())
			{
				return Attached;
			}
		}
	}

	return nullptr;
}


FVector UEMRGameplayAbility_PickItem::ComputeDropLocation(const ACharacter& Character) const
{
	const FVector ForwardOffset = Character.GetActorForwardVector() * 85.f;
	const FVector Start         = Character.GetActorLocation() + ForwardOffset + FVector(0.f, 0.f, 60.f);
	const FVector End           = Start - FVector(0.f, 0.f, 300.f);

	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemBoxDrop), false, &Character);

	if (UWorld* World = GetWorld())
	{
		World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
	}

	return HitResult.bBlockingHit ? HitResult.Location : Start;
}
