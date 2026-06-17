

#include "MobaCameraAimComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "MobaPlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Camera/CameraComponent.h"
#include "Data/MobaCameraAimingProfile.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "GAS/MobaGameplayTags.h"


UMobaCameraAimComponent::UMobaCameraAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}




void UMobaCameraAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCamera.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}
	
	
	if (!IsCameraTransitionFinished())
	{
		MoveCameraToTargetLocation(DeltaTime);	
	}
}




void UMobaCameraAimComponent::MoveCameraToTargetLocation(float DeltaTime)
{
	const float Alpha = FMath::Clamp(DeltaTime * AimTransitionSpeed, 0.f, 1.f);
	NewCameraRelativeLocation = FMath::Lerp(CachedCameraRelativeLocation, CameraTargetRelativeLocation, Alpha);
	OwnerCamera->SetRelativeLocation(NewCameraRelativeLocation);
}




bool UMobaCameraAimComponent::IsCameraTransitionFinished()
{
	CachedCameraRelativeLocation = OwnerCamera->GetRelativeLocation();
	
	if (FVector::Dist(CachedCameraRelativeLocation, CameraTargetRelativeLocation) <= KINDA_SMALL_NUMBER)
	{
		OwnerCamera->SetRelativeLocation(CameraTargetRelativeLocation);
		SetComponentTickEnabled(false);
		return true;
	}
	return false;
}




void UMobaCameraAimComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = GetOwner<AMobaPlayerCharacter>();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled()) { return; }
	
	OwnerCamera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!OwnerCamera.IsValid()) { return; }

	OwnerASC = Cast<UMobaAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter));
	if (!OwnerASC) { return; }

	
	CameraDefaultRelativeLocation = OwnerCamera->GetRelativeLocation();
	
	OwnerASC->RegisterGameplayTagEvent(MobaAbilityAimingEventTags::Ability_Aiming, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnAimingTagChanged);
}




void UMobaCameraAimComponent::OnAimingTagChanged(const FGameplayTag AimingTag, const int32 AimingTagCount)
{
	if (!OwnerCamera.IsValid() || !OwnerASC) { return; }
	
	bIsAiming = AimingTagCount > 0;
	
	if (bIsAiming)
	{
		FindAimingProfileBasedOnChildGameplayTag();
		
		if (!CameraAimingProfile) { return; }
		
		CameraTargetRelativeLocation = CameraAimingProfile->TargetCameraOffset;
		AimTransitionSpeed = CameraAimingProfile->InTransitionSpeed;
	}
	else
	{
		CameraAimingProfile = CameraAimProfileMap.FindRef(LastAimingTag);
		if (CameraAimingProfile)
		{
			CameraTargetRelativeLocation = CameraDefaultRelativeLocation;
			AimTransitionSpeed = CameraAimingProfile->OutTransitionSpeed;

			LastAimingTag = FGameplayTag::EmptyTag;	
		}
	}
	
	SetComponentTickEnabled(true);
}




void UMobaCameraAimComponent::FindAimingProfileBasedOnChildGameplayTag()
{
	FGameplayTagContainer OwnedTags;
	OwnerASC->GetOwnedGameplayTags(OwnedTags);

	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(MobaAbilityAimingEventTags::Ability_Aiming) && !Tag.MatchesTagExact(MobaAbilityAimingEventTags::Ability_Aiming))
		{
			CameraAimingProfile = CameraAimProfileMap.FindRef(Tag);
			if (!CameraAimingProfile) { continue; }
			LastAimingTag = Tag;
				
			break;
		}
	}
}


