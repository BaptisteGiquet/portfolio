
#include "Subsystems/EMRNavigationIntentSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "EngineUtils.h"
#include "Framework/EMRAssetManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/EMRBaseMachine.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRSubsystemConfig.h"

void UEMRNavigationIntentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadSubsystemConfig();
}


void UEMRNavigationIntentSubsystem::Deinitialize()
{
    PatientTagListeners.Empty();
    CachedNavigationMappings.Empty();

    Super::Deinitialize();
}


void UEMRNavigationIntentSubsystem::RegisterPatient(AEMRPatient* Patient)
{
    if (!Patient || !Patient->HasAuthority())
    {
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    if (CachedNavigationMappings.IsEmpty())
    {
        BuildNavigationMappingsCache(nullptr);
    }

    RegisterTagListeners(Patient, ASC);
}


void UEMRNavigationIntentSubsystem::LoadSubsystemConfig()
{
    UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::OnLoadedSubsystemConfig));
}


void UEMRNavigationIntentSubsystem::OnLoadedSubsystemConfig()
{
    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);

    if (LoadedSubsystemConfig.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavigationIntentSubsystem] SubsystemConfig not loaded"));
        return;
    }

    UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetNavigationIntentMappingTable();
    if (!MappingTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavigationIntentSubsystem] NavigationIntentMappingTable is null"));
        return;
    }

    BuildNavigationMappingsCache(MappingTable);
}


void UEMRNavigationIntentSubsystem::BuildNavigationMappingsCache(UDataTable* MappingTable)
{
    CachedNavigationMappings.Empty();

    if (!MappingTable)
    {
        return;
    }

    static const FString Context = TEXT("NavigationIntentSubsystem");
    TArray<FEMRNavigationTargetData*> Rows;
    MappingTable->GetAllRows(Context, Rows);

    for (const FEMRNavigationTargetData* Row : Rows)
    {
        if (!Row || !Row->TriggerTag.IsValid())
        {
            continue;
        }

        CachedNavigationMappings.Add(Row->TriggerTag, *Row);
    }

}


void UEMRNavigationIntentSubsystem::RegisterTagListeners(AEMRPatient* Patient, UAbilitySystemComponent* ASC)
{
    if (!Patient || !ASC)
    {
        return;
    }

    TArray<FEMRNavigationTagListener> Listeners;

    for (const TPair<FGameplayTag, FEMRNavigationTargetData>& Pair : CachedNavigationMappings)
    {
        if (!Pair.Key.IsValid())
        {
            continue;
        }

        FOnGameplayEffectTagCountChanged& Delegate = ASC->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved);
        FDelegateHandle Handle = Delegate.AddUObject(this, &ThisClass::HandlePatientTagChanged, Patient);

        FEMRNavigationTagListener Listener;
        Listener.ObservedTag = Pair.Key;
        Listener.DelegateHandle = Handle;

        Listeners.Add(Listener);
    }

    PatientTagListeners.Add(Patient, Listeners);
}


void UEMRNavigationIntentSubsystem::HandlePatientTagChanged(const FGameplayTag Tag, int32 NewCount, AEMRPatient* Patient)
{
    if (!Patient || NewCount <= 0)
    {
        return;
    }

    const FEMRNavigationTargetData* Mapping = CachedNavigationMappings.Find(Tag);
    if (!Mapping)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavigationIntentSubsystem] No navigation mapping found for tag %s"), *Tag.ToString());
        return;
    }

    PublishNavigationIntent(Patient, *Mapping);
}


void UEMRNavigationIntentSubsystem::PublishNavigationIntent(AEMRPatient* Patient, const FEMRNavigationTargetData& Mapping)
{
    if (!Patient)
    {
        return;
    }

    FEMRNavigationIntentMessage Message;
    Message.Patient = Patient;
    Message.TriggerTag = Mapping.TriggerTag;
    Message.MachineTypeTag = Mapping.MachineTypeTag;
    Message.MessageTag = Mapping.MessageTag.IsValid() ? Mapping.MessageTag : Mapping.TriggerTag;

    if (AActor* Target = ResolveNavigationTarget(Patient, Mapping))
    {
        Message.TargetActor = Target;
        Message.TargetLocation = Target->GetActorLocation();
        Message.TargetRotation = Target->GetActorRotation();
    }

    BroadcastNavigationIntent(Patient, Message.MessageTag, Message.TargetActor.Get(), Message.TargetLocation, Message.TargetRotation, Message.MachineTypeTag);
}


AActor* UEMRNavigationIntentSubsystem::ResolveNavigationTarget(AEMRPatient* Patient, const FEMRNavigationTargetData& Mapping) const
{
    if (!Patient)
    {
        return nullptr;
    }

    if (Mapping.TargetActor.IsValid())
    {
        return Mapping.TargetActor.Get();
    }

    if (!Mapping.TargetActor.IsNull())
    {
        return Mapping.TargetActor.LoadSynchronous();
    }

    if (Mapping.MachineTypeTag.IsValid())
    {
        return FindClosestMachine(Patient, Mapping.MachineTypeTag);
    }

    return nullptr;
}


AEMRBaseMachine* UEMRNavigationIntentSubsystem::FindClosestMachine(AActor* OriginActor, const FGameplayTag& MachineTypeTag) const
{
	if (!OriginActor || !MachineTypeTag.IsValid())
	{
		return nullptr;
	}

	const UWorld* World = OriginActor->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AEMRBaseMachine* ClosestMachine = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
	{
		if (!It->GetMachineTypeTag().MatchesTag(MachineTypeTag))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OriginActor->GetActorLocation(), It->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestMachine = *It;
		}
	}

	return ClosestMachine;
}


void UEMRNavigationIntentSubsystem::BroadcastNavigationIntent(AEMRPatient* Patient, const FGameplayTag& MessageTag, AActor* TargetActor, const FVector& TargetLocation, const FRotator& TargetRotation, const FGameplayTag& MachineTypeTag)
{
    if (!Patient || !MessageTag.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);

    FEMRNavigationIntentMessage Message;
    Message.Patient = Patient;
    Message.MessageTag = MessageTag;
    Message.MachineTypeTag = MachineTypeTag;
    Message.TriggerTag = MessageTag;

    if (TargetActor)
    {
        Message.TargetActor = TargetActor;

        Message.TargetLocation = TargetLocation.IsNearlyZero() ? TargetActor->GetActorLocation() : TargetLocation;
        Message.bHasTargetLocation = true;

        Message.TargetRotation = TargetRotation.IsNearlyZero() ? TargetActor->GetActorRotation() : TargetRotation;
        Message.bHasTargetRotation = true;
    }
    else
    {
        if (!TargetLocation.IsNearlyZero())
        {
            Message.TargetLocation = TargetLocation;
            Message.bHasTargetLocation = true;
        }

        if (!TargetRotation.IsNearlyZero())
        {
            Message.TargetRotation = TargetRotation;
            Message.bHasTargetRotation = true;
        }
    }

    MessageSubsystem.BroadcastMessage(MessageTag, Message);
}


void UEMRNavigationIntentSubsystem::BroadcastNavigationIntentToMachine(AEMRPatient* Patient, const FGameplayTag& MessageTag, const FGameplayTag& MachineTypeTag)
{
    if (!Patient || !MachineTypeTag.IsValid())
    {
        return;
    }

    AActor* TargetActor = FindClosestMachine(Patient, MachineTypeTag);
    FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
    FRotator TargetRotation = TargetActor ? TargetActor->GetActorRotation() : FRotator::ZeroRotator;

    BroadcastNavigationIntent(Patient, MessageTag, TargetActor, TargetLocation, TargetRotation, MachineTypeTag);
}


FGameplayTag UEMRNavigationIntentSubsystem::GetNavigationMessageTagForMachine(const FGameplayTag& MachineTypeTag) const
{
    if (!MachineTypeTag.IsValid())
    {
        return EMRTags::GMS::AI::Navigation::Root;
    }

    for (const TPair<FGameplayTag, FEMRNavigationTargetData>& Pair : CachedNavigationMappings)
    {
        const FEMRNavigationTargetData& Mapping = Pair.Value;
        if (!Mapping.MachineTypeTag.IsValid())
        {
            continue;
        }

        if (MachineTypeTag.MatchesTag(Mapping.MachineTypeTag) || Mapping.MachineTypeTag.MatchesTag(MachineTypeTag))
        {
            return Mapping.MessageTag.IsValid() ? Mapping.MessageTag : Mapping.TriggerTag;
        }
    }

    return EMRTags::GMS::AI::Navigation::Root;
}

