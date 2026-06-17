#include "Characters/Patient/EMRPatient.h"

#include "GAS/EMRAbilitySystemComponent.h"
#include "GAS/EMRTags.h"
#include "Data/EMRAttributeToTagMapping.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StreamableManager.h"
#include "Framework/EMRAssetManager.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "GAS/Attributes/EMRPatientAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Framework/EMRNightShiftGameMode.h"
#include "NavMesh/RecastNavMesh.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"


namespace
{
	constexpr float BasePatienceDrainPerSecond = 1.0f;

	constexpr uint8 GetPatienceSuppressionSourceMask(const EEMRPatienceDrainSuppressor Source)
	{
		return static_cast<uint8>(1u << static_cast<uint8>(Source));
	}
}



AEMRPatient::AEMRPatient()
{
    AbilitySystemComponent = CreateDefaultSubobject<UEMRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UEMRPatientAttributeSet>(TEXT("AttributeSet"));

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));

	GetCapsuleComponent()->InitCapsuleSize(25.f, 92.f);

    if (UCharacterMovementComponent* PatientMovement = GetCharacterMovement())
    {
	    PatientMovement->bUseRVOAvoidance = false;

    	FNavAgentProperties& NavProps = PatientMovement->NavAgentProps;

    	NavProps.AgentRadius                   = 15.f;
    	NavProps.AgentHeight                   = 95.f;
    	NavProps.AgentStepHeight               = -1.f;   
    	NavProps.NavWalkingSearchHeightScale   = 0.5f;
    	NavProps.PreferredNavData              = ARecastNavMesh::StaticClass();

    	
    	NavProps.bCanCrouch = true;
    	NavProps.bCanJump   = true;
    	NavProps.bCanWalk   = true;
    	NavProps.bCanSwim   = true;
    	NavProps.bCanFly    = false;
    }
}


void AEMRPatient::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PatientDataID, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, bCanReceiveTreatment, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME(ThisClass, FullName);
    DOREPLIFETIME(ThisClass, Age);
    DOREPLIFETIME(ThisClass, BloodType);
    DOREPLIFETIME(ThisClass, CurrentAssignedMachine);
    DOREPLIFETIME(ThisClass, NextMachineOnList);
    DOREPLIFETIME(ThisClass, bFolderDisplayRegistered);
    DOREPLIFETIME(ThisClass, bIsSeated);
    DOREPLIFETIME(ThisClass, SeatedAnimationTag);
}


UAbilitySystemComponent* AEMRPatient::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


FGameplayEventData AEMRPatient::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    if (InteractableComponent)
    {
        FGameplayEventData EventData = InteractableComponent->BuildInteractionEventData(InterActor);
        EventData.Instigator = InterActor;
        EventData.Target = this;
        return EventData;
    }

    return FGameplayEventData();
}


FText AEMRPatient::GetInteractableDisplayName_Implementation() const
{
    if (!FullName.IsEmpty())
    {
        return FullName;
    }

    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}


bool AEMRPatient::IsInteractableEnabled_Implementation() const
{
    return false;
}


void AEMRPatient::SetIsSeated(const bool bInIsSeated)
{
    if (!HasAuthority() || bIsSeated == bInIsSeated)
    {
        return;
    }

    bIsSeated = bInIsSeated;
    if (!bIsSeated)
    {
        SeatedAnimationTag = FGameplayTag();
    }
}


void AEMRPatient::SetSeatedAnimationState(const bool bInIsSeated, const FGameplayTag& InSeatedAnimationTag)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!bInIsSeated)
    {
        ClearSeatedAnimationState();
        return;
    }

    SeatedAnimationTag = InSeatedAnimationTag;
    SetIsSeated(true);
}


void AEMRPatient::ClearSeatedAnimationState()
{
    if (!HasAuthority())
    {
        return;
    }

    SetIsSeated(false);
    SeatedAnimationTag = FGameplayTag();
}


void AEMRPatient::SetSeatedAnimationTag_Implementation(const FGameplayTag& InTag)
{
    if (!HasAuthority())
    {
        return;
    }

    SeatedAnimationTag = InTag;
}


FGameplayTag AEMRPatient::GetSeatedAnimationTag_Implementation() const
{
    return SeatedAnimationTag;
}


void AEMRPatient::OnRep_IsSeated()
{
}


void AEMRPatient::OnRep_SeatedAnimationTag()
{
}


void AEMRPatient::BeginPlay()
{
    Super::BeginPlay();
    EnsureCapsuleRenderingHidden();

    if (HasAuthority())
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        BuildAttributeToTagMap();
        RegisterPatienceChangeDelegate();

        if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
        {
            NavigationSubsystem->RegisterPatient(this);
        }
    }
}


void AEMRPatient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        UnregisterPatienceChangeDelegate();

        if (UWorld* World = GetWorld())
        {
            if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
            {
                SpawnSubsystem->NotifyActivePatientReleased();
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}


void AEMRPatient::InitializeFromPatientData(const UEMRPatientData* InPatientData)
{
	if (!InPatientData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EMRPatientCharacter] Attempted to initialize with null PatientData"));
		return;
	}

    PatientData = InPatientData;

    if (HasAuthority())
    {
        PatientDataID = InPatientData->GetPrimaryAssetId();
        bFolderDisplayRegistered = false;
        GenerateIdentityFromData();
        CacheRewardValues();

        ApplyPatientAttributes();
        RegisterPatienceChangeDelegate();
        GrantStartupAbilities();
        GrantStartupEffects();

        float OvertimeMultiplier = 1.0f;
        if (const UWorld* World = GetWorld())
        {
            if (const AEMRNightShiftGameState* RunGS = World->GetGameState<AEMRNightShiftGameState>())
            {
                OvertimeMultiplier = RunGS->GetOvertimePatienceDrainMultiplier();
            }
        }

        RefreshPatienceDrainEffect(OvertimeMultiplier);
        ApplyPathologyTags();
    }
}


UStaticMeshComponent* AEMRPatient::GetOrCreateUltrasoundSkinProxyComponent()
{
    USkeletalMeshComponent* ParentMesh = nullptr;
    if (UltrasoundSkinProxyParentMeshTag != NAME_None)
    {
        const TArray<UActorComponent*> TaggedComponents = GetComponentsByTag(
            USkeletalMeshComponent::StaticClass(),
            UltrasoundSkinProxyParentMeshTag);
        if (TaggedComponents.Num() > 0)
        {
            ParentMesh = Cast<USkeletalMeshComponent>(TaggedComponents[0]);
        }
    }

    if (!ParentMesh && UltrasoundSkinProxyParentMeshName != NAME_None)
    {
        TArray<USkeletalMeshComponent*> MeshComponents;
        GetComponents<USkeletalMeshComponent>(MeshComponents);
        for (USkeletalMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent && MeshComponent->GetFName() == UltrasoundSkinProxyParentMeshName)
            {
                ParentMesh = MeshComponent;
                break;
            }
        }
    }

    if (!ParentMesh)
    {
        ParentMesh = GetMesh();
    }
    if (!ParentMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UltrasoundTrace] Patient %s missing ultrasound parent mesh; cannot attach proxy."), *GetName());
        return UltrasoundSkinProxyComponent;
    }

    const auto AlignProxyToParent = [this, ParentMesh]()
    {
        if (!UltrasoundSkinProxyComponent)
        {
            return;
        }

        const FTransform DesiredWorldTransform = UltrasoundSkinProxyOffset * ParentMesh->GetComponentTransform();
        UltrasoundSkinProxyComponent->SetWorldTransform(DesiredWorldTransform);

    };

    if (UltrasoundSkinProxyComponent || !UltrasoundSkinProxyMesh)
    {
        if (!UltrasoundSkinProxyMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("[UltrasoundTrace] Patient %s missing UltrasoundSkinProxyMesh."), *GetName());
        }

        if (UltrasoundSkinProxyComponent)
        {
            AlignProxyToParent();
            ApplyUltrasoundSkinProxyVisibilityPolicy();
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Patient %s updated proxy transform parent=%s world=%s rel=%s"),
                *GetName(),
                *GetNameSafe(ParentMesh),
                *UltrasoundSkinProxyComponent->GetComponentLocation().ToCompactString(),
                *UltrasoundSkinProxyComponent->GetRelativeLocation().ToCompactString());
        }

        return UltrasoundSkinProxyComponent;
    }

    UltrasoundSkinProxyComponent = NewObject<UStaticMeshComponent>(this, TEXT("UltrasoundSkinProxy"));
    UltrasoundSkinProxyComponent->SetStaticMesh(UltrasoundSkinProxyMesh);
    UltrasoundSkinProxyComponent->SetupAttachment(ParentMesh);
    UltrasoundSkinProxyComponent->SetRelativeTransform(UltrasoundSkinProxyOffset);
    UltrasoundSkinProxyComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UltrasoundSkinProxyComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    UltrasoundSkinProxyComponent->SetMobility(EComponentMobility::Movable);
    UltrasoundSkinProxyComponent->SetGenerateOverlapEvents(false);
    UltrasoundSkinProxyComponent->RegisterComponent();
    ApplyUltrasoundSkinProxyVisibilityPolicy();
    AlignProxyToParent();
    
    return UltrasoundSkinProxyComponent;
}


bool AEMRPatient::ShouldShowUltrasoundSkinProxy() const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    return bShowUltrasoundSkinProxy;
#endif
}


void AEMRPatient::ApplyUltrasoundSkinProxyVisibilityPolicy()
{
    if (!UltrasoundSkinProxyComponent)
    {
        return;
    }

    const bool bShouldShowProxy = ShouldShowUltrasoundSkinProxy();
    UltrasoundSkinProxyComponent->SetVisibility(bShouldShowProxy, true);
    UltrasoundSkinProxyComponent->SetHiddenInGame(!bShouldShowProxy, true);
}


void AEMRPatient::ApplyPathologyTags()
{
    if (!PatientData || !HasAuthority())
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> TagEffect = PatientData->GetAddTagToPatientEffect();
    if (!TagEffect)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] AddTagToPatientEffect not defined for %s"), *GetName());
        return;
    }

    const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
    if (Pathologies.IsEmpty())
    {
        return;
    }

    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    for (const FGameplayTag& PathologyTag : Pathologies)
    {
        if (!PathologyTag.IsValid())
        {
            continue;
        }

        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(TagEffect, 1.0f, EffectContext);
        if (SpecHandle.IsValid())
        {
            SpecHandle.Data->DynamicGrantedTags.AddTag(PathologyTag);
            ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }
}


void AEMRPatient::LoadPatientDataFromID()
{
	if (!PatientDataID.IsValid())
	{
		return;
	}

	UEMRAssetManager& AssetManager = UEMRAssetManager::Get();
	FStreamableDelegate LoadDelegate;
	LoadDelegate.BindLambda([this]()
	{
		UObject* LoadedAsset = UEMRAssetManager::Get().GetPrimaryAssetObject(PatientDataID);
		PatientData = Cast<UEMRPatientData>(LoadedAsset);

        if (PatientData)
        {
                BuildAttributeToTagMap();
        }
        else
        {
	        UE_LOG(LogTemp, Error, TEXT("[Client] Failed to load PatientData from ID: %s"), *PatientDataID.ToString());
        }
    });

	AssetManager.LoadPrimaryAsset(PatientDataID, TArray<FName>(), LoadDelegate);
}


void AEMRPatient::ApplyPatientAttributes()
{
    if (!PatientData || !GetAbilitySystemComponent() || !HasAuthority())
    {
		return;
	}

	TSubclassOf<UGameplayEffect> InitEffect = PatientData->GetInitializationEffect();
	if (!InitEffect)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponent()->MakeEffectContext();
	EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(
            InitEffect,
            1.0f,
            EffectContext
    );

    if (SpecHandle.IsValid())
    {
        const float InitialPatience = GetInitialPatienceForSeverity();
        const FGameplayTag PatienceTag = GetAttributeTag(UEMRPatientAttributeSet::GetPatienceAttribute());
        const FGameplayTag MaxPatienceTag = GetAttributeTag(UEMRPatientAttributeSet::GetMaxPatienceAttribute());

        const TArray<FEMRAttributeValue> VitalStats = PatientData->GetDefaultVitalStats();
        if (VitalStats.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Server] No vital stats defined for patient data %s"), *PatientData->GetName());
        }

        for (const FEMRAttributeValue& Stat : VitalStats)
        {
            if (Stat.Attribute.IsValid())
            {
                if (Stat.Attribute == UEMRPatientAttributeSet::GetPatienceAttribute() || Stat.Attribute == UEMRPatientAttributeSet::GetMaxPatienceAttribute())
                {
                    continue;
                }

                FGameplayTag AttributeTag = GetAttributeTag(Stat.Attribute);
                if (AttributeTag.IsValid())
                {
                    SpecHandle.Data->SetSetByCallerMagnitude(AttributeTag, Stat.Value);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[Server] Missing tag mapping for attribute %s on patient %s"),
                            *Stat.Attribute.GetName(), *GetName());
                }
            }
        }

        if (InitialPatience > 0.0f)
    {
            if (PatienceTag.IsValid())
            {
                SpecHandle.Data->SetSetByCallerMagnitude(PatienceTag, InitialPatience);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[Server] Missing tag mapping for patience attribute on patient %s"), *GetName());
            }

            if (MaxPatienceTag.IsValid())
            {
                SpecHandle.Data->SetSetByCallerMagnitude(MaxPatienceTag, InitialPatience);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[Server] Missing tag mapping for max patience attribute on patient %s"), *GetName());
            }
        }

        GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}


void AEMRPatient::GrantStartupAbilities()
{
	if (!PatientData || !GetAbilitySystemComponent() || !HasAuthority())
	{
		return;
	}

	
	const TArray<TSubclassOf<UGameplayAbility>>& Abilities = PatientData->GetStartupAbilities();
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Abilities)
	{
		if (AbilityClass)
		{
			if (!GetAbilitySystemComponent()->FindAbilitySpecFromClass(AbilityClass))
			{
				GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}
	}
}


void AEMRPatient::GrantStartupEffects()
{
    if (!PatientData || !GetAbilitySystemComponent() || !HasAuthority())
        {
            return;
    }

	const TArray<TSubclassOf<UGameplayEffect>>& Effects = PatientData->GetStartupEffects();
	for (const TSubclassOf<UGameplayEffect>& Effect : Effects)
	{
		if (Effect)
		{
			FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponent()->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(
				Effect,
				1.0f,
				EffectContext
			);

			if (SpecHandle.IsValid())
			{
				GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                        }
                }
    }
}


void AEMRPatient::RefreshPatienceDrainEffect(float OvertimeMultiplier)
{
    if (!HasAuthority())
        {
			return;
    }

    CachedPatienceDrainMultiplier = FMath::Max(OvertimeMultiplier, 0.0f);

    if (!PatientData || !GetAbilitySystemComponent())
        {
            return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ActivePatienceDrainEffectHandle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(ActivePatienceDrainEffectHandle);
            ActivePatienceDrainEffectHandle.Invalidate();
    }

    if (bPatienceDrainSuppressed)
        {
            return;
    }

    TSubclassOf<UGameplayEffect> PatienceDrainEffect = PatientData->GetPatienceDrainEffect();
    if (!PatienceDrainEffect)
        {
            return;
    }

    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(PatienceDrainEffect, CachedPatienceDrainMultiplier, EffectContext);
    if (SpecHandle.IsValid())
        {
            SpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::PatienceDrain, -BasePatienceDrainPerSecond * CachedPatienceDrainMultiplier);
            ActivePatienceDrainEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}

void AEMRPatient::SetPatienceDrainSuppressed(const bool bSuppressed)
{
    SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::Legacy, bSuppressed);
}

void AEMRPatient::SetPatienceDrainSuppressedBySource(const EEMRPatienceDrainSuppressor Source, const bool bSuppressed)
{
    if (!HasAuthority())
    {
        return;
    }

    const uint8 SourceMask = GetPatienceSuppressionSourceMask(Source);
    const bool bWasSuppressedBySource = (PatienceDrainSuppressionSourceMask & SourceMask) != 0;
    if (bWasSuppressedBySource == bSuppressed)
    {
        return;
    }

    if (bSuppressed)
    {
        PatienceDrainSuppressionSourceMask |= SourceMask;
    }
    else
    {
        PatienceDrainSuppressionSourceMask &= static_cast<uint8>(~SourceMask);
    }

    const bool bWasSuppressed = bPatienceDrainSuppressed;
    bPatienceDrainSuppressed = PatienceDrainSuppressionSourceMask != 0;
    if (bWasSuppressed != bPatienceDrainSuppressed)
    {
        RefreshPatienceDrainEffect(CachedPatienceDrainMultiplier);
    }
}


void AEMRPatient::PrimeMeshAndAnimationForSpawn()
{
    EnsureCapsuleRenderingHidden();

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    GetComponents(SkeletalMeshComponents);

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        SkeletalMeshComponent->SetComponentTickEnabled(true);
        SkeletalMeshComponent->SetVisibility(true, true);
        SkeletalMeshComponent->SetHiddenInGame(false, true);
        SkeletalMeshComponent->UpdateBounds();
    }
}


void AEMRPatient::EnsureCapsuleRenderingHidden()
{
    if (UCapsuleComponent* PatientCapsuleComponent = GetCapsuleComponent())
    {
        PatientCapsuleComponent->SetVisibility(false, false);
        PatientCapsuleComponent->SetHiddenInGame(true, false);
    }

    TArray<UArrowComponent*> ArrowComponents;
    GetComponents(ArrowComponents);
    for (UArrowComponent* PatientArrowComponent : ArrowComponents)
    {
        if (!PatientArrowComponent)
        {
            continue;
        }

        PatientArrowComponent->SetVisibility(false, false);
        PatientArrowComponent->SetHiddenInGame(true, false);
    }
}

void AEMRPatient::SetCurrentAssignedMachine(AEMRBaseMachine* InMachine)
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentAssignedMachine = InMachine;
}

void AEMRPatient::SetNextMachineOnList(AEMRBaseMachine* InMachine)
{
    if (!HasAuthority())
    {
        return;
    }

    NextMachineOnList = InMachine;
}

void AEMRPatient::SetFolderDisplayRegistered(bool bInRegistered)
{
    if (!HasAuthority())
    {
        return;
    }

    bFolderDisplayRegistered = bInRegistered;
}


void AEMRPatient::ResetForPooling()
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->CancelAllAbilities();
        ASC->ClearAllAbilities();
        ASC->RemoveAllGameplayCues();

            const FGameplayEffectQuery RemoveAllEffects = FGameplayEffectQuery::MakeQuery_MatchAllEffectTags(FGameplayTagContainer());
        ASC->RemoveActiveEffects(RemoveAllEffects);

        FGameplayTagContainer OwnedTags;
        ASC->GetOwnedGameplayTags(OwnedTags);
        for (const FGameplayTag& Tag : OwnedTags)
        {
            ASC->RemoveLooseGameplayTag(Tag);
        }
        ASC->ForceReplication();
    }

    TArray<UActorComponent*> Components;
    GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (Component && Component->ComponentHasTag(EMRTags::Component::TreatmentAttachment.GetModuleName()))
        {
            Component->DestroyComponent();
        }
    }

    UnregisterPatienceChangeDelegate();

    PatientData = nullptr;
    PatientDataID = FPrimaryAssetId();
    bCanReceiveTreatment = false;
    bFolderDisplayRegistered = false;
    FullName = FText();
    Age = 0;
    BloodType = FGameplayTag::EmptyTag;
    CurrentAssignedMachine = nullptr;
    NextMachineOnList = nullptr;
    bHasCachedRewards = false;
    CachedMoneyReward = 0.0f;
    CachedReputationReward = 0.0f;
    bPatienceDrainSuppressed = false;
    PatienceDrainSuppressionSourceMask = 0;
    CachedPatienceDrainMultiplier = 1.0f;
    ActivePatienceDrainEffectHandle.Invalidate();
    CachedDifficultyTuning.Reset();
    bIsSeated = false;
    SeatedAnimationTag = FGameplayTag();
}


void AEMRPatient::RegisterPatienceChangeDelegate()
{
    if (PatienceChangeDelegateHandle.IsValid() || !HasAuthority())
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        PatienceChangeDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UEMRPatientAttributeSet::GetPatienceAttribute())
                .AddUObject(this, &ThisClass::HandlePatienceChanged);
    }
}

void AEMRPatient::UnregisterPatienceChangeDelegate()
{
    if (!PatienceChangeDelegateHandle.IsValid())
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UEMRPatientAttributeSet::GetPatienceAttribute()).Remove(PatienceChangeDelegateHandle);
    }

    PatienceChangeDelegateHandle = FDelegateHandle();
}

void AEMRPatient::HandlePatienceChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!HasAuthority() || ChangeData.NewValue > 0.f)
    {
        return;
    }

    UnregisterPatienceChangeDelegate();

    if (UWorld* World = GetWorld())
    {
        if (AEMRNightShiftGameMode* NightShiftGameMode = World->GetAuthGameMode<AEMRNightShiftGameMode>())
        {
            NightShiftGameMode->HandlePatientOutOfPatience(this);
        }
    }
}


void AEMRPatient::BuildAttributeToTagMap()
{
    if (!CachedAttributeToTagMap.IsEmpty()) return;
	
	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
	UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
	
	if (LoadedSubsystemConfig.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] SubsystemConfig not loaded"));
		return;
	}

	UDataTable* AttributeToTagMap = LoadedSubsystemConfig[0]->GetAttributeToTagMapping();
	if (!AttributeToTagMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] AttributeToTagMap is not set"));
		return;
	}

	static const FString Context(TEXT("AttributeToTagMap"));
	TArray<FEMRAttributeToTagMapping*> Rows;
	AttributeToTagMap->GetAllRows(Context, Rows);

	for (const FEMRAttributeToTagMapping* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		CachedAttributeToTagMap.FindOrAdd(Row->Attribute) = Row->Tag;	
	}
}


FGameplayTag AEMRPatient::GetAttributeTag(const FGameplayAttribute& Attribute) const
{
    if (CachedAttributeToTagMap.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] CachedAttributeToTagMap is empty, call BuildAttributeToTagMap first"));
        return FGameplayTag::EmptyTag;
    }

    const FGameplayTag* Tag = CachedAttributeToTagMap.Find(Attribute);
    return Tag ? *Tag : FGameplayTag::EmptyTag;
}


void AEMRPatient::OnRep_PatientDataID()
{
    if (!PatientDataID.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Client] Received invalid PatientDataID"));
        return;
    }
    LoadPatientDataFromID();

    BuildAttributeToTagMap();
}


void AEMRPatient::GenerateIdentityFromData()
{
    if (!PatientData)
    {
        return;
    }

    const FText RandomName = PatientData->GetRandomName();
    if (!RandomName.IsEmpty())
    {
        FullName = RandomName;
    }

    Age = PatientData->GetRandomAge();

    const FGameplayTag RandomBloodType = PatientData->GetRandomBloodType();
    BloodType = RandomBloodType.IsValid() ? RandomBloodType : FGameplayTag::EmptyTag;
}


const UEMRDifficultyTuningData* AEMRPatient::GetDifficultyTuning() const
{
    if (CachedDifficultyTuning.IsValid())
    {
        return CachedDifficultyTuning.Get();
    }

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);

    if (LoadedSubsystemConfig.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] SubsystemConfig not loaded, cannot fetch DifficultyTuning"));
        return nullptr;
    }

    CachedDifficultyTuning = LoadedSubsystemConfig[0]->GetDifficultyTuning();
    if (!CachedDifficultyTuning.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] DifficultyTuning not set in SubsystemConfig"));
    }

    return CachedDifficultyTuning.Get();
}

void AEMRPatient::CacheRewardValues()
{
    bHasCachedRewards = false;
    CachedMoneyReward = 0.0f;
    CachedReputationReward = 0.0f;

    if (!PatientData)
    {
        return;
    }

    const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning();
    if (!DifficultyTuning)
    {
        return;
    }

    const FGameplayTag Severity = PatientData->GetSeverity();
    CachedMoneyReward = FMath::Max(0.0f, DifficultyTuning->GetPatientMoneyReward(Severity));
    CachedReputationReward = FMath::Max(0.0f, DifficultyTuning->GetPatientReputationReward(Severity));
    bHasCachedRewards = true;
}


float AEMRPatient::GetInitialPatienceForSeverity() const
{
    if (!PatientData)
    {
        return 0.0f;
    }

    const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning();
    if (!DifficultyTuning)
    {
        return 0.0f;
    }

    const FGameplayTag Severity = PatientData->GetSeverity();
    const float InitialPatience = DifficultyTuning->GetPatientInitialPatience(Severity);

    if (InitialPatience <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatient] Initial patience missing or zero for severity %s on patient %s"), *Severity.ToString(), *GetName());
    }

    return InitialPatience;
}
