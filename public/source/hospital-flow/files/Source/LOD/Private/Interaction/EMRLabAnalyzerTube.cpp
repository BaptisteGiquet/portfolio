#include "Interaction/EMRLabAnalyzerTube.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRLabAnalyzerMachine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

AEMRLabAnalyzerTube::AEMRLabAnalyzerTube()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AEMRLabAnalyzerTube::BeginPlay()
{
    Super::BeginPlay();

    EMR_LAB_LOG(Log, "Tube BeginPlay %s", *GetNameSafe(this));

    BuildTestDefinitionCache();
    CacheTestDecals();
    UpdateDecalMarkers();

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetUseObjectEventTag(EMRTags::Event::LabAnalyzer::UseTube);
        Carryable->SetPlaceObjectEventTag(EMRTags::Event::Item::Place);
    }

    if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
    {
        Interactable->SetInteractionEventTag(EMRTags::Event::Item::Pick);
        if (Interactable->GetDisplayName().IsEmpty())
        {
            Interactable->SetDisplayName(FText::FromString(TEXT("Blood Tube")));
        }
    }
}

void AEMRLabAnalyzerTube::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindFromPatient();

    Super::EndPlay(EndPlayReason);
}

void AEMRLabAnalyzerTube::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        return;
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        const bool bIsCarriedNow = Carryable->IsCarried();
        if (bIsCarriedNow && !bWasCarriedAny)
        {
            EMR_LAB_LOG(Log, "Tube picked up %s", *GetNameSafe(this));
        }
        else if (!bIsCarriedNow && bWasCarriedAny)
        {
            EMR_LAB_LOG(Log, "Tube dropped %s", *GetNameSafe(this));
        }

        bWasCarriedAny = bIsCarriedNow;
    }

    UpdateCarryState();
}

void AEMRLabAnalyzerTube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRLabAnalyzerTube, RequiredTests);
    DOREPLIFETIME(AEMRLabAnalyzerTube, CompletedTests);
    DOREPLIFETIME(AEMRLabAnalyzerTube, bIsAbandoned);
    DOREPLIFETIME(AEMRLabAnalyzerTube, bIsBalanceTube);
    DOREPLIFETIME(AEMRLabAnalyzerTube, bInteractionLocked);
    DOREPLIFETIME(AEMRLabAnalyzerTube, bInsertedInMachine);
    DOREPLIFETIME(AEMRLabAnalyzerTube, bStoredInRack);
    DOREPLIFETIME(AEMRLabAnalyzerTube, OwningPatient);
}

void AEMRLabAnalyzerTube::InitializeTube(AEMRPatient* Patient, const FGameplayTagContainer& InRequiredTests)
{
    if (!HasAuthority())
    {
        return;
    }

    OwningPatient = Patient;
    RequiredTests = InRequiredTests;
    CompletedTests.Reset();
    bIsAbandoned = false;
    bExamCompletionTriggered = false;

    EMR_LAB_LOG(Log, "Tube Initialize patient=%s tests=%s", *GetNameSafe(Patient), *RequiredTests.ToStringSimple());
    BindToPatient(Patient);
    OnRep_TubeState();
    ForceNetUpdate();
}

void AEMRLabAnalyzerTube::MarkExamCompletionTriggered()
{
    if (!HasAuthority() || bExamCompletionTriggered)
    {
        return;
    }

    bExamCompletionTriggered = true;
}

void AEMRLabAnalyzerTube::MarkTestsCompleted(const FGameplayTagContainer& TestsToComplete)
{
    if (!HasAuthority() || bIsBalanceTube || bIsAbandoned)
    {
        return;
    }

    EMR_LAB_LOG(Log, "Tube MarkTestsCompleted tube=%s tests=%s", *GetNameSafe(this), *TestsToComplete.ToStringSimple());
    bool bUpdated = false;
    for (const FGameplayTag& TestTag : TestsToComplete)
    {
        if (!TestTag.IsValid())
        {
            continue;
        }

        if (!RequiredTests.HasTagExact(TestTag))
        {
            continue;
        }

        if (!CompletedTests.HasTagExact(TestTag))
        {
            CompletedTests.AddTag(TestTag);
            bUpdated = true;
        }
    }

    if (bUpdated)
    {
        OnRep_TubeState();
        ForceNetUpdate();
    }
}

bool AEMRLabAnalyzerTube::AreAllTestsCompleted() const
{
    if (RequiredTests.IsEmpty())
    {
        return true;
    }

    for (const FGameplayTag& TestTag : RequiredTests)
    {
        if (!CompletedTests.HasTagExact(TestTag))
        {
            return false;
        }
    }

    return true;
}

bool AEMRLabAnalyzerTube::CanBeTrashed() const
{
    if (bIsBalanceTube)
    {
        return false;
    }

    return bIsAbandoned || AreAllTestsCompleted();
}

void AEMRLabAnalyzerTube::MarkAbandoned()
{
    if (!HasAuthority() || bIsAbandoned)
    {
        return;
    }

    bIsAbandoned = true;
    OwningPatient = nullptr;

    EMR_LAB_LOG(Log, "Tube MarkAbandoned %s", *GetNameSafe(this));
    UnbindFromPatient();

    OnRep_TubeState();
    ForceNetUpdate();
}

void AEMRLabAnalyzerTube::SetBalanceTube(bool bInBalance)
{
    if (!HasAuthority())
    {
        return;
    }

    bIsBalanceTube = bInBalance;
    EMR_LAB_LOG(Log, "Tube SetBalanceTube %s balance=%s", *GetNameSafe(this), bIsBalanceTube ? TEXT("true") : TEXT("false"));
    OnRep_TubeState();
    ForceNetUpdate();
}

void AEMRLabAnalyzerTube::SetInteractionLocked(bool bLocked)
{
    if (bInteractionLocked == bLocked)
    {
        return;
    }

    bInteractionLocked = bLocked;
    EMR_LAB_LOG(Log, "Tube InteractionLocked %s locked=%s", *GetNameSafe(this), bInteractionLocked ? TEXT("true") : TEXT("false"));

    if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
    {
        Interactable->SetEnabled(!bLocked);
    }

    OnRep_TubeState();
    ForceNetUpdate();
}

void AEMRLabAnalyzerTube::SetInsertedInMachine(AEMRLabAnalyzerMachine* Machine)
{
    CurrentMachine = Machine;
    bInsertedInMachine = (Machine != nullptr);
    bWasCarried = false;
    if (bInsertedInMachine)
    {
        bStoredInRack = false;
    }

    EMR_LAB_LOG(Log, "Tube SetInsertedInMachine %s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    ApplyInsertionState();
    OnRep_TubeState();
    ForceNetUpdate();
}

void AEMRLabAnalyzerTube::SetStoredInRack(bool bInStored, USceneComponent* AttachAnchor)
{
    if (!HasAuthority())
    {
        return;
    }

    bStoredInRack = bInStored;

    if (bStoredInRack && AttachAnchor)
    {
        if (UPrimitiveComponent* TubePrimitive = Cast<UPrimitiveComponent>(GetRootComponent()))
        {
            TubePrimitive->SetSimulatePhysics(false);
        }

        AttachToComponent(AttachAnchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        SetActorTransform(AttachAnchor->GetComponentTransform());
        SetReplicateMovement(false);
    }

    ApplyInsertionState();
    OnRep_TubeState();
    ForceNetUpdate();
}

bool AEMRLabAnalyzerTube::IsInteractableEnabled_Implementation() const
{
    if (bInteractionLocked)
    {
        return false;
    }

    return Super::IsInteractableEnabled_Implementation();
}

void AEMRLabAnalyzerTube::OnRep_TubeState()
{
    if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
    {
        Interactable->SetEnabled(!bInteractionLocked);
    }

    CacheTestDecals();
    UpdateDecalMarkers();
    ApplyInsertionState();
    HandleTubeStateChanged();
}

void AEMRLabAnalyzerTube::BuildTestDefinitionCache()
{
    CachedTestDefinitions.Reset();

    if (!TestDefinitionsTable)
    {
        return;
    }

    static const FString ContextString(TEXT("LabAnalyzerTube"));
    const TArray<FName> RowNames = TestDefinitionsTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        const FEMRLabAnalyzerTestDefinition* Row = TestDefinitionsTable->FindRow<FEMRLabAnalyzerTestDefinition>(RowName, ContextString);
        if (!Row || !Row->TestTag.IsValid())
        {
            continue;
        }

        CachedTestDefinitions.Add(Row->TestTag, *Row);
    }
}

void AEMRLabAnalyzerTube::CacheTestDecals()
{
    CachedTestDecals.Reset();

    TInlineComponentArray<UDecalComponent*> Decals(this);
    GetComponents(Decals);

    for (UDecalComponent* Decal : Decals)
    {
        if (!Decal)
        {
            continue;
        }

        if (!TestDecalTag.IsNone() && !Decal->ComponentHasTag(TestDecalTag))
        {
            continue;
        }

        CachedTestDecals.Add(Decal);
    }

    if (bSortDecalsByName)
    {
        CachedTestDecals.Sort([](const UDecalComponent& A, const UDecalComponent& B)
        {
            return A.GetFName().LexicalLess(B.GetFName());
        });
    }

    EMR_LAB_LOG(Log, "Tube cached decals=%d tag=%s", CachedTestDecals.Num(), *TestDecalTag.ToString());
}

void AEMRLabAnalyzerTube::GetTestDecalComponents(TArray<UDecalComponent*>& OutDecals) const
{
    OutDecals.Reset();
}

int32 AEMRLabAnalyzerTube::GetSortOrderForTag(const FGameplayTag& TestTag) const
{
    if (const FEMRLabAnalyzerTestDefinition* Found = CachedTestDefinitions.Find(TestTag))
    {
        return Found->SortOrder;
    }

    return MAX_int32;
}

FLinearColor AEMRLabAnalyzerTube::GetColorForTag(const FGameplayTag& TestTag) const
{
    if (const FEMRLabAnalyzerTestDefinition* Found = CachedTestDefinitions.Find(TestTag))
    {
        return Found->TestColor;
    }

    return FLinearColor::White;
}

void AEMRLabAnalyzerTube::UpdateDecalMarkers()
{
    UpdateDecalMarkersForComponents(CachedTestDecals, DecalMIDs);
}

void AEMRLabAnalyzerTube::UpdateDecalMarkersForComponents(const TArray<UDecalComponent*>& Decals, TArray<TObjectPtr<UMaterialInstanceDynamic>>& InOutMIDs)
{
    InOutMIDs.Reset();
    for (UDecalComponent* Decal : Decals)
    {
        if (Decal)
        {
            Decal->SetVisibility(false, true);
            Decal->SetHiddenInGame(true, true);
        }
    }
}

void AEMRLabAnalyzerTube::ApplyInsertionState()
{
    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetLockedInPlace(bInsertedInMachine || bStoredInRack);
    }
}
void AEMRLabAnalyzerTube::BindToPatient(AEMRPatient* Patient)
{
    UnbindFromPatient();

    if (!IsValid(Patient))
    {
        return;
    }

    if (Patient->IsActorBeingDestroyed())
    {
        MarkAbandoned();
        return;
    }

    if (UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent())
    {
        PatientLeavingHandle = PatientASC->RegisterGameplayTagEvent(EMRTags::Patient::Phase::Leaving, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &ThisClass::HandlePatientLeaving);
    }

    Patient->OnDestroyed.AddDynamic(this, &ThisClass::HandlePatientDestroyed);

    if (UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent())
    {
        if (PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
        {
            MarkAbandoned();
        }
    }
}

void AEMRLabAnalyzerTube::UnbindFromPatient()
{
    AEMRPatient* Patient = OwningPatient.Get();
    if (Patient)
    {
        Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);

        if (UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent())
        {
            if (PatientLeavingHandle.IsValid())
            {
                PatientASC->RegisterGameplayTagEvent(EMRTags::Patient::Phase::Leaving).Remove(PatientLeavingHandle);
                PatientLeavingHandle.Reset();
            }
        }
    }
    else
    {
        PatientLeavingHandle.Reset();
    }
}

void AEMRLabAnalyzerTube::HandlePatientLeaving(const FGameplayTag CallbackTag, int32 NewCount)
{
    if (NewCount > 0)
    {
        EMR_LAB_LOG(Log, "Tube patient leaving detected %s", *GetNameSafe(this));
        MarkAbandoned();
    }
}

void AEMRLabAnalyzerTube::HandlePatientDestroyed(AActor* DestroyedActor)
{
    EMR_LAB_LOG(Log, "Tube patient destroyed %s", *GetNameSafe(this));
    MarkAbandoned();
}

void AEMRLabAnalyzerTube::UpdateCarryState()
{
    if (!CurrentMachine.IsValid())
    {
        bInsertedInMachine = false;
        ApplyInsertionState();
    }

    UEMRCarryableComponent* Carryable = GetCarryableComponent();
    const bool bIsCarriedNow = Carryable && Carryable->IsCarried();

    if (bStoredInRack && bIsCarriedNow)
    {
        bStoredInRack = false;
        ApplyInsertionState();
        ForceNetUpdate();
    }

    if (!bInsertedInMachine)
    {
        return;
    }

    if (bIsCarriedNow && !bWasCarried)
    {
        EMR_LAB_LOG(Log, "Tube picked up from machine %s", *GetNameSafe(this));
        CurrentMachine->HandleTubePickedUp(this);
        bInsertedInMachine = false;
        CurrentMachine.Reset();
    }

    bWasCarried = bIsCarriedNow;
}
