#include "GAS/Abilities/EMRGameplayAbility_SelectNightShift.h"

#include "AbilitySystemComponent.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRHubGameMode.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"


UEMRGameplayAbility_SelectNightShift::UEMRGameplayAbility_SelectNightShift()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::SelectNightShift;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    ActivationBlockedTags.AddTag(EMRTags::GameMode::Hub::PendingNightShift);
}


void UEMRGameplayAbility_SelectNightShift::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] ActivateAbility - TriggerTag=%s"), TriggerEventData ? *TriggerEventData->EventTag.ToString() : TEXT("None"));

    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] CommitAbility failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!GetAvatarActorFromActorInfo())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	
    const int32 NightShiftIndex = TriggerEventData ? FMath::RoundToInt(TriggerEventData->EventMagnitude) : INDEX_NONE;
    if (NightShiftIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] Invalid offer index: %d"), NightShiftIndex);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UEMRNightShiftDefinition* NightShiftDefinition = ResolveDefinitionFromOfferIndex(ActorInfo, NightShiftIndex);
    if (!NightShiftDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] ResolveDefinitionFromOfferIndex failed for %d"), NightShiftIndex);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!ValidateSelection(ActorInfo, NightShiftDefinition))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] ValidateSelection failed for %s"), *NightShiftDefinition->GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] Selection accepted for %s"), *NightShiftDefinition->GetName());
    CacheSelectionOnServer(ActorInfo, NightShiftDefinition);

    AEMRNightShiftGameState* NightShiftGameState = nullptr;
    if (const UWorld* World = GetWorld())
    {
        NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>();
    }

    if (NightShiftGameState)
    {
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] Committing selection on GameState"));
        NightShiftGameState->SetNightShiftSelectionCommitted(true);
    }
	
	AController* Controller = GetAvatarActorFromActorInfo()->GetInstigatorController();
	
    AEMRPlayerState* PlayerState = Cast<AEMRPlayerState>(Controller->GetPlayerState<AEMRPlayerState>());
    if (PlayerState)
    {
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] Applying PendingNightShift tag to %s"), *PlayerState->GetPlayerName());
        ApplyPendingNightShiftTag(PlayerState);
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] Ending ability"));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


void UEMRGameplayAbility_SelectNightShift::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    ClearAllPendingTags();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UEMRGameplayAbility_SelectNightShift::ApplyPendingNightShiftTag(AEMRPlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> EffectClass = PlayerState->GetAddTagsToPlayerEffect();
    if (!EffectClass)
    {
        return;
    }

    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
    if (!SpecHandle.IsValid())
    {
        return;
    }

    SpecHandle.Data->DynamicGrantedTags.AddTag(SelectionLockedTag);
    FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    FNighShiftPendingTagHandles* Entry = PendingTagEffects.FindByPredicate([PlayerState](const FNighShiftPendingTagHandles& Item)
    {
        return Item.PlayerState == PlayerState;
    });

    if (!Entry)
    {
        FNighShiftPendingTagHandles NewEntry;
        NewEntry.PlayerState = PlayerState;
        Entry = &PendingTagEffects.Add_GetRef(NewEntry);
    }

    Entry->Handles.Add(ActiveHandle);
}


void UEMRGameplayAbility_SelectNightShift::RemovePendingNightShiftTags(AEMRPlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    if (const FNighShiftPendingTagHandles* Entry = PendingTagEffects.FindByPredicate([PlayerState](const FNighShiftPendingTagHandles& Item)
    {
        return Item.PlayerState == PlayerState;
    }))
    {
        for (const FActiveGameplayEffectHandle& Handle : Entry->Handles)
        {
            ASC->RemoveActiveGameplayEffect(Handle);
        }
    }
}


void UEMRGameplayAbility_SelectNightShift::ClearAllPendingTags() const
{
    for (const FNighShiftPendingTagHandles& Entry : PendingTagEffects)
    {
        if (AEMRPlayerState* PlayerState = Entry.PlayerState.Get())
        {
            UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
            if (!ASC)
            {
                continue;
            }

            for (const FActiveGameplayEffectHandle& Handle : Entry.Handles)
            {
                ASC->RemoveActiveGameplayEffect(Handle);
            }
        }
    }

    PendingTagEffects.Reset();
}


bool UEMRGameplayAbility_SelectNightShift::ValidateSelection(const FGameplayAbilityActorInfo* ActorInfo, UEMRNightShiftDefinition* Definition) const
{
    if (!ActorInfo || !GetAvatarActorFromActorInfo())
    {
        return false;
    }

    const UWorld* World = GetAvatarActorFromActorInfo()->GetWorld();
    AEMRNightShiftGameState* NightShiftGameState = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (!NightShiftGameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_SelectNightShift] NightShiftGameState unavailable."));
        return false;
    }

    if (NightShiftGameState->IsNightShiftSelectionCommitted())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_SelectNightShift] Selection rejected: already committed."));
        return false;
    }

    if (NightShiftGameState->GetRunPhase() != EER_RunPhase::Hub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_SelectNightShift] Selection rejected: phase is not Hub."));
        return false;
    }

    const TArray<UEMRNightShiftDefinition*>& AvailableNightShifts = NightShiftGameState->GetAvailableNextNightShifts();
    if (!AvailableNightShifts.Contains(Definition))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_SelectNightShift] Selection rejected: %s not in offers."), *Definition->GetName());
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] ValidateSelection succeeded for %s"), *Definition->GetName());
    return true;
}


UEMRNightShiftDefinition* UEMRGameplayAbility_SelectNightShift::ResolveDefinitionFromOfferIndex(const FGameplayAbilityActorInfo* ActorInfo, int32 OfferIndex) const
{
    if (!ActorInfo || !GetAvatarActorFromActorInfo())
    {
        return nullptr;
    }

    if (OfferIndex < 0)
    {
        return nullptr;
    }

    const UWorld* World = GetAvatarActorFromActorInfo()->GetWorld();
    const AEMRNightShiftGameState* NightShiftGameState = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (!NightShiftGameState)
    {
        return nullptr;
    }

    const TArray<UEMRNightShiftDefinition*>& Offers = NightShiftGameState->GetAvailableNextNightShifts();
    return Offers.IsValidIndex(OfferIndex) ? Offers[OfferIndex] : nullptr;
}


void UEMRGameplayAbility_SelectNightShift::CacheSelectionOnServer(const FGameplayAbilityActorInfo* ActorInfo, UEMRNightShiftDefinition* Definition) const
{
    if (!ActorInfo || !GetAvatarActorFromActorInfo())
    {
        return;
    }

    UWorld* World = GetAvatarActorFromActorInfo()->GetWorld();
    if (!World)
    {
        return;
    }

    if (AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>())
    {
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][GA] Forwarding selection to HubGameMode"));
        HubGameMode->StartNightShiftFromSelection(Definition);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] HubGameMode unavailable (likely client-only PIE). Cancelling ability."));
    CancelAbilityWithError(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, TEXT("HubGameModeMissing"));
}


void UEMRGameplayAbility_SelectNightShift::CancelAbilityWithError(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    FName ErrorTag) const
{
    UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][GA] Cancelling ability due to error: %s"), *ErrorTag.ToString());
    const_cast<UEMRGameplayAbility_SelectNightShift*>(this)->EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
