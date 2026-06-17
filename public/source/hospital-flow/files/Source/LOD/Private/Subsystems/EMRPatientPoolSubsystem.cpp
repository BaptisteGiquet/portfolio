#include "Subsystems/EMRPatientPoolSubsystem.h"

#include "AIController.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Framework/EMRAssetManager.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "GameFramework/Actor.h"
#include "Environment/EMRPatientPoolAnchor.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Interaction/EMRBaseMachine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/AssetManager.h"
#include "EngineUtils.h"


namespace
{
    static constexpr ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}


bool UEMRPatientPoolSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}


void UEMRPatientPoolSubsystem::InitializePool(int32 Size)
{
    UpdateStorageLocationFromGameState();
    TargetPoolSize = FMath::Max(Size, 0);

    InactivePatients.RemoveAllSwap([](const TWeakObjectPtr<AEMRPatient>& Patient)
    {
        return !Patient.IsValid();
    });

    for (auto It = PatientDataByPatient.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }

    if (TargetPoolSize == 0)
    {
        DestroyPooledPatients();
        return;
    }

    if (PatientClasses.Num() == 0)
    {
        PatientClasses.Add(AEMRPatient::StaticClass());
    }

    const int32 CurrentPooledCount = InactivePatients.Num();
    const int32 Missing = TargetPoolSize - CurrentPooledCount;
    for (int32 Index = 0; Index < Missing; ++Index)
    {
        if (AEMRPatient* Patient = SpawnPatient())
        {
            DeactivatePatient(*Patient);
            InactivePatients.Add(Patient);
        }
    }
}

AEMRPatient* UEMRPatientPoolSubsystem::AcquirePatient(bool bActivate)
{
    RefreshPatientDataCache();

    if (CachedPatientData.Num() == 0 && !bHasWarnedMissingPatientData)
    {
        bHasWarnedMissingPatientData = true;
        UE_LOG(LogTemp, Warning, TEXT("[PatientPool] No patient data assets are loaded; patients will spawn without data until assets are available."));
    }

    AEMRPatient* Patient = AcquirePatientShell();
    if (Patient)
    {
        const UEMRPatientData* PatientData = SelectPatientDataForAutomaticSpawn();
        if (PatientData)
        {
            ApplyPatientDataForAutomaticSpawn(*Patient, *PatientData);
            bHasWarnedMissingPatientData = false;
        }

        EnablePatientAIForAutomaticSpawn(*Patient);

        if (bActivate)
        {
            ActivatePatient(*Patient, Patient->GetActorTransform());
        }
        else
        {
            DeactivatePatient(*Patient);
        }
        return Patient;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[PatientPool] AcquirePatient failed: could not reuse or spawn patient (Inactive=%d, TargetPoolSize=%d, PatientClasses=%d)."),
        InactivePatients.Num(),
        TargetPoolSize,
        PatientClasses.Num());

    return nullptr;
}

AEMRPatient* UEMRPatientPoolSubsystem::AcquirePatientShell()
{
    while (InactivePatients.Num() > 0)
    {
        TWeakObjectPtr<AEMRPatient> Candidate = InactivePatients.Pop();
        if (!Candidate.IsValid())
        {
            continue;
        }

        if (AEMRPatient* Patient = Candidate.Get())
        {
            DeactivatePatient(*Patient);
            return Patient;
        }
    }

    if (AEMRPatient* SpawnedPatient = SpawnPatient())
    {
        DeactivatePatient(*SpawnedPatient);
        return SpawnedPatient;
    }

    return nullptr;
}

const UEMRPatientData* UEMRPatientPoolSubsystem::SelectPatientDataForAutomaticSpawn() const
{
    const_cast<UEMRPatientPoolSubsystem*>(this)->RefreshPatientDataCache();
    return SelectPatientDataWeighted();
}

bool UEMRPatientPoolSubsystem::ApplyPatientDataForAutomaticSpawn(AEMRPatient& Patient, const UEMRPatientData& PatientData)
{
    Patient.InitializeFromPatientData(&PatientData);
    PatientDataByPatient.Add(&Patient, &PatientData);
    ApplyPatienceDrainMultiplierToPatient(Patient);
    return true;
}

void UEMRPatientPoolSubsystem::EnablePatientAIForAutomaticSpawn(AEMRPatient& Patient) const
{
    if (AAIController* AIController = Cast<AAIController>(Patient.GetController()))
    {
        if (AEMRAIController* EMRController = Cast<AEMRAIController>(AIController))
        {
            EMRController->RestartBehaviorTree();
        }
    }
}

void UEMRPatientPoolSubsystem::UpdateStorageLocationFromGameState()
{
    if (const UWorld* World = GetWorld())
    {
        if (CachedPoolAnchor.IsValid())
        {
            StorageLocation = CachedPoolAnchor->GetPoolLocation();
            return;
        }

        for (TActorIterator<AEMRPatientPoolAnchor> It(World); It; ++It)
        {
            if (AEMRPatientPoolAnchor* PoolAnchor = *It)
            {
                CachedPoolAnchor = PoolAnchor;
                StorageLocation = PoolAnchor->GetPoolLocation();
                return;
            }
        }

        if (const AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>())
        {
            StorageLocation = NightShiftGameState->GetPatientPoolLocation();
        }
    }
}

void UEMRPatientPoolSubsystem::ActivatePatient(AEMRPatient& Patient, const FTransform& SpawnTransform)
{
    Patient.SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (UCharacterMovementComponent* PatientMovement = Patient.GetCharacterMovement())
    {
        PatientMovement->SetMovementMode(MOVE_Walking);
        PatientMovement->StopMovementImmediately();
    }
    Patient.SetActorHiddenInGame(false);
    Patient.SetActorEnableCollision(true);
    Patient.EnsureCapsuleRenderingHidden();
    Patient.ForceNetUpdate();
    Patient.FlushNetDormancy();
}


void UEMRPatientPoolSubsystem::ReleasePatient(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
        {
            if (AEMRWaitingRoomArea* WaitingArea = *It)
            {
                if (UEMRWaitingRoomManagerComponent* Manager = WaitingArea->GetManagerComponent())
                {
                    Manager->RemovePatientFromQueue(Patient);
                    break;
                }
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UEMRExamQueueSubsystem* ExamQueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
            {
                UE_LOG(LogTemp, Warning, TEXT("[ExamRoutingDebug] Pool release queue cleanup: patient=%s"), *GetNameSafe(Patient));
                ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
        {
            if (AEMRBaseMachine* Machine = *It)
            {
                if (Machine->GetAssignedPatient() == Patient)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("[ExamRoutingDebug] Pool release machine cleanup: patient=%s machine=%s machineTag=%s"),
                        *GetNameSafe(Patient),
                        *GetNameSafe(Machine),
                        *Machine->GetMachineTypeTag().ToString());
                    Machine->ClearOccupiedPatient(Patient);
                }
            }
        }
    }

    if (AEMRAIController* PatientController = Patient->GetController<AEMRAIController>())
    {
        PatientController->ResetBlackboardState();
    }

    Patient->ResetForPooling();
    DeactivatePatient(*Patient);
    PatientDataByPatient.Remove(Patient);
    InactivePatients.Add(Patient);
}


void UEMRPatientPoolSubsystem::PrewarmForNightShift(const UEMRNightShiftDefinition* Definition)
{
    CurrentNightShiftDefinition = Definition;
    ClearSpecialEventPathologyWeights();

    LoadDifficultyTuning();
    BasePatienceDrainMultiplier = 1.0f;
    if (Definition && CachedDifficultyTuning)
    {
        BasePatienceDrainMultiplier = CachedDifficultyTuning->GetPatienceDrainMultiplier(Definition->DifficultyTier);
    }

    PrewarmAutomaticSpawnResources();

    const int32 WarmTarget = Definition ? GetAutomaticWarmActorTarget() : FMath::Max(TargetPoolSize, 0);
    InitializePool(WarmTarget);

    RefreshPatientDataCache();
    ApplyPatienceDrainMultiplier(1.0f);
}

void UEMRPatientPoolSubsystem::PrewarmAutomaticSpawnResources()
{
    WarmConfiguredPatientClassesAsync();
    WarmPatientDataAssets();
}

void UEMRPatientPoolSubsystem::SetSpecialEventPathologyWeights(
    const TArray<FEMRWeightedPathologyEntry>& WeightedPathologies,
    const float NonMatchingWeight)
{
    ActiveSpecialEventPathologyWeights.Reset();
    ActiveSpecialEventNonMatchingWeight = FMath::Max(NonMatchingWeight, 0.0f);
    bSpecialEventPathologyOverrideActive = false;

    for (const FEMRWeightedPathologyEntry& Entry : WeightedPathologies)
    {
        if (!Entry.PathologyTag.IsValid())
        {
            continue;
        }

        const float Weight = FMath::Max(Entry.Weight, 0.0f);
        if (Weight <= 0.0f)
        {
            continue;
        }

        float& AggregatedWeight = ActiveSpecialEventPathologyWeights.FindOrAdd(Entry.PathologyTag);
        AggregatedWeight += Weight;
    }

    bSpecialEventPathologyOverrideActive = ActiveSpecialEventPathologyWeights.Num() > 0;
}

void UEMRPatientPoolSubsystem::ClearSpecialEventPathologyWeights()
{
    ActiveSpecialEventPathologyWeights.Reset();
    ActiveSpecialEventNonMatchingWeight = 1.0f;
    bSpecialEventPathologyOverrideActive = false;
}


FEMRPatientPoolDebugInfo UEMRPatientPoolSubsystem::GetDebugInfo() const
{
    FEMRPatientPoolDebugInfo Info;
    Info.TargetPoolSize = TargetPoolSize;
    Info.InactivePatients = InactivePatients.Num();
    Info.ActiveTrackedPatients = PatientDataByPatient.Num();
    Info.CachedPatientData = CachedPatientData.Num();
    if (PatientClasses.Num() == 0)
    {
        Info.PatientClassName = TEXT("None");
    }
    else
    {
        TArray<FString> ClassNames;
        ClassNames.Reserve(PatientClasses.Num());
        for (const TSubclassOf<AEMRPatient>& Class : PatientClasses)
        {
            ClassNames.Add(Class ? Class->GetName() : TEXT("None"));
        }

        Info.PatientClassName = FString::Join(ClassNames, TEXT(", "));
    }
    Info.CurrentNightShiftName = CurrentNightShiftDefinition ? CurrentNightShiftDefinition->GetName() : TEXT("None");
    return Info;
}


void UEMRPatientPoolSubsystem::DestroyPooledPatients()
{
    for (const TWeakObjectPtr<AEMRPatient>& Patient : InactivePatients)
    {
        if (Patient.IsValid())
        {
            Patient->Destroy();
        }
    }

    InactivePatients.Reset();
    PatientDataByPatient.Reset();
    CachedPatientData.Reset();
}

void UEMRPatientPoolSubsystem::ApplyPatienceDrainMultiplier(float Multiplier)
{
    if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
    {
        return;
    }

    const float EffectiveMultiplier = BasePatienceDrainMultiplier * Multiplier;
    CurrentPatienceDrainMultiplier = EffectiveMultiplier;

    for (TPair<TWeakObjectPtr<AEMRPatient>, TObjectPtr<const UEMRPatientData>>& Entry : PatientDataByPatient)
    {
        if (AEMRPatient* Patient = Entry.Key.Get())
        {
            Patient->RefreshPatienceDrainEffect(EffectiveMultiplier);
        }
    }
}


void UEMRPatientPoolSubsystem::RefreshPatientDataCache()
{
    TArray<const UEMRPatientData*> LoadedPatients;
    UEMRAssetManager::Get().CollectLoadedPatients(LoadedPatients);

    CachedPatientData.Reset();
    for (const UEMRPatientData* PatientData : LoadedPatients)
    {
        CachedPatientData.Add(PatientData);
    }
}

void UEMRPatientPoolSubsystem::WarmConfiguredPatientClassesAsync()
{
    if (PatientClasses.Num() == 0)
    {
        return;
    }

    const int32 BatchSize = FMath::Max(AutomaticWarmClassBatchSize, 1);
    FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

    for (int32 StartIndex = 0; StartIndex < PatientClasses.Num(); StartIndex += BatchSize)
    {
        TArray<FSoftObjectPath> BatchPaths;
        const int32 EndIndex = FMath::Min(StartIndex + BatchSize, PatientClasses.Num());
        BatchPaths.Reserve(EndIndex - StartIndex);

        for (int32 ClassIndex = StartIndex; ClassIndex < EndIndex; ++ClassIndex)
        {
            const TSubclassOf<AEMRPatient> PatientClass = PatientClasses[ClassIndex];
            if (!PatientClass)
            {
                continue;
            }

            if (const UClass* PatientClassObject = PatientClass.Get())
            {
                const FSoftObjectPath ClassPath(PatientClassObject);
                if (ClassPath.IsValid())
                {
                    BatchPaths.Add(ClassPath);
                }
            }
        }

        if (BatchPaths.Num() > 0)
        {
            StreamableManager.RequestAsyncLoad(BatchPaths, FStreamableDelegate());
        }
    }
}

void UEMRPatientPoolSubsystem::WarmPatientDataAssets()
{
    UEMRAssetManager::Get().LoadPatients(FStreamableDelegate::CreateUObject(this, &ThisClass::RefreshPatientDataCache));
    RefreshPatientDataCache();
}


void UEMRPatientPoolSubsystem::LoadDifficultyTuning()
{
    if (CachedDifficultyTuning)
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.Num() == 0)
    {
        return;
    }

    const UEMRSubsystemConfig* Config = Configs[0];
    if (Config)
    {
        CachedDifficultyTuning = Config->GetDifficultyTuning();
    }
}


const UEMRPatientData* UEMRPatientPoolSubsystem::SelectPatientData() const
{
    if (CachedPatientData.Num() == 0)
    {
        return nullptr;
    }

    const int32 Index = FMath::RandRange(0, CachedPatientData.Num() - 1);
    return CachedPatientData[Index];
}


const UEMRPatientData* UEMRPatientPoolSubsystem::SelectPatientDataWeighted() const
{
    if (CachedPatientData.Num() == 0)
    {
        return nullptr;
    }

    // If no NightShift definition, fallback to uniform random selection
    if (!CurrentNightShiftDefinition)
    {
        return SelectPatientData();
    }

    // Build weighted array based on difficulty-based spawn weights
    TArray<float> Weights;
    Weights.Reserve(CachedPatientData.Num());

    float TotalWeight = 0.0f;

    for (const UEMRPatientData* PatientData : CachedPatientData)
    {
        if (!PatientData)
        {
            Weights.Add(0.0f);
            continue;
        }

        const FGameplayTag PatientSeverityTag = PatientData->GetSeverity();
        const EEMRNightShiftDifficultyTier NightShiftDifficulty = CurrentNightShiftDefinition->DifficultyTier;

        float Weight = CachedDifficultyTuning
            ? CachedDifficultyTuning->GetPatientSpawnWeight(NightShiftDifficulty, PatientSeverityTag)
            : 1.0f;

        if (bSpecialEventPathologyOverrideActive)
        {
            const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
            float EventWeight = 0.0f;

            for (const TPair<FGameplayTag, float>& PathologyWeight : ActiveSpecialEventPathologyWeights)
            {
                if (Pathologies.HasTagExact(PathologyWeight.Key))
                {
                    EventWeight += PathologyWeight.Value;
                }
            }

            if (EventWeight <= 0.0f)
            {
                EventWeight = ActiveSpecialEventNonMatchingWeight;
            }

            Weight *= FMath::Max(EventWeight, 0.0f);
        }

        Weights.Add(Weight);
        TotalWeight += Weight;
    }

    // Weighted random selection
    if (TotalWeight <= 0.0f)
    {
        return SelectPatientData(); // Fallback to uniform
    }

    const float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
    float Accumulator = 0.0f;

    for (int32 Index = 0; Index < CachedPatientData.Num(); ++Index)
    {
        Accumulator += Weights[Index];
        if (RandomValue <= Accumulator)
        {
            return CachedPatientData[Index];
        }
    }

    // Fallback (should never happen)
    return CachedPatientData.Last();
}

AEMRPatient* UEMRPatientPoolSubsystem::SpawnPatient() const
{
    const TSubclassOf<AEMRPatient> PatientClassToSpawn = SelectPatientClass();
    UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PatientPool] SpawnPatient failed: World is null."));
        return nullptr;
    }

    if (!PatientClassToSpawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PatientPool] SpawnPatient failed: selected patient class is null."));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;
    AEMRPatient* SpawnedPatient = World->SpawnActor<AEMRPatient>(PatientClassToSpawn, FTransform(StorageLocation), SpawnParams);
    if (!SpawnedPatient)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[PatientPool] SpawnActor failed for class %s at Loc=(%.1f, %.1f, %.1f)."),
            *GetNameSafe(PatientClassToSpawn.Get()),
            StorageLocation.X,
            StorageLocation.Y,
            StorageLocation.Z);
    }

    return SpawnedPatient;
}

void UEMRPatientPoolSubsystem::DeactivatePatient(AEMRPatient& Patient) const
{
    Patient.SetActorLocation(StorageLocation);
    if (UCharacterMovementComponent* PatientMovement = Patient.GetCharacterMovement())
    {
        PatientMovement->StopMovementImmediately();
        PatientMovement->DisableMovement();
    }
    Patient.SetActorHiddenInGame(true);
    Patient.SetActorEnableCollision(false);
    Patient.EnsureCapsuleRenderingHidden();
    Patient.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Patient.ForceNetUpdate();
}

void UEMRPatientPoolSubsystem::ApplyPatienceDrainMultiplierToPatient(AEMRPatient& Patient)
{
    if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
    {
        return;
    }

    Patient.RefreshPatienceDrainEffect(CurrentPatienceDrainMultiplier);
}




void UEMRPatientPoolSubsystem::SetPatientClasses(const TArray<TSubclassOf<AEMRPatient>>& InClasses)
{
    PatientClasses.Reset();

    for (const TSubclassOf<AEMRPatient>& Class : InClasses)
    {
        if (Class)
        {
            PatientClasses.Add(Class);
        }
    }

    if (PatientClasses.Num() == 0)
    {
        PatientClasses.Add(AEMRPatient::StaticClass());
        UE_LOG(LogTemp, Warning, TEXT("[PatientPool] SetPatientClasses received no valid classes, falling back to AEMRPatient."));
    }

    TArray<FString> PatientClassNames;
    PatientClassNames.Reserve(PatientClasses.Num());
    for (const TSubclassOf<AEMRPatient>& PatientClass : PatientClasses)
    {
        PatientClassNames.Add(GetNameSafe(PatientClass.Get()));
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[PatientPool] SetPatientClasses Input=%d Resolved=%d [%s]"),
        InClasses.Num(),
        PatientClasses.Num(),
        PatientClassNames.Num() > 0 ? *FString::Join(PatientClassNames, TEXT(", ")) : TEXT("<none>"));
}

TSubclassOf<AEMRPatient> UEMRPatientPoolSubsystem::SelectPatientClass() const
{
    if (PatientClasses.Num() == 0)
    {
        return AEMRPatient::StaticClass();
    }

    const int32 Index = FMath::RandRange(0, PatientClasses.Num() - 1);
    return PatientClasses[Index];
}
