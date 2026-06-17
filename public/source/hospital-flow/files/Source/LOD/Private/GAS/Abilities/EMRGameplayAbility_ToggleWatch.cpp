#include "GAS/Abilities/EMRGameplayAbility_ToggleWatch.h"

#include "AbilitySystemComponent.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectGlobals.h"

UEMRGameplayAbility_ToggleWatch::UEMRGameplayAbility_ToggleWatch()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UEMRGameplayAbility_ToggleWatch::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    ActiveToggleWatchMontage.Reset();

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] CommitAbility failed."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!ActorInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] ActorInfo null."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetAvatarActorFromActorInfo());
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Avatar is not AEMRPlayerCharacter."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    USkeletalMeshComponent* AbilityMesh = ActorInfo->SkeletalMeshComponent.Get();
    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    const FString OwnerName = ActorInfo->OwnerActor.IsValid() ? ActorInfo->OwnerActor->GetName() : TEXT("None");
    const FString AvatarName = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetName() : TEXT("None");
    const FString ASCName = ASC ? ASC->GetName() : TEXT("None");
    const FString MeshName = AbilityMesh ? AbilityMesh->GetName() : TEXT("None");
    const FString AnimName = AnimInstance ? AnimInstance->GetName() : TEXT("None");
    UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Owner=%s Avatar=%s Local=%d Role=%d ASC=%s Mesh=%s Anim=%s Tag=%s"),
        *OwnerName,
        *AvatarName,
        ActorInfo->IsLocallyControlled() ? 1 : 0,
        ActorInfo->OwnerActor.IsValid() ? static_cast<int32>(ActorInfo->OwnerActor->GetLocalRole()) : -1,
        *ASCName,
        *MeshName,
        *AnimName,
        *ActorInfo->AffectedAnimInstanceTag.ToString());

    if (AbilityMesh)
    {
        const USkeletalMesh* MeshAsset = AbilityMesh->GetSkeletalMeshAsset();
        const FString MeshAssetName = MeshAsset ? MeshAsset->GetName() : TEXT("None");
        const FString AnimClassName = AbilityMesh->GetAnimClass() ? AbilityMesh->GetAnimClass()->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] MeshAsset=%s AnimClass=%s"),
            *MeshAssetName,
            *AnimClassName);
    }

    if (!AnimInstance)
    {
        USkeletalMeshComponent* FallbackMesh = nullptr;
        if (PlayerCharacter)
        {
            FallbackMesh = PlayerCharacter->GetPlayerMeshComponentForSeating();
            if (!FallbackMesh)
            {
                FallbackMesh = PlayerCharacter->GetMesh();
            }
        }

        if (FallbackMesh && FallbackMesh != AbilityMesh)
        {
            AbilityMesh = FallbackMesh;
            AnimInstance = AbilityMesh->GetAnimInstance();

            const USkeletalMesh* MeshAsset = AbilityMesh->GetSkeletalMeshAsset();
            const FString MeshAssetName = MeshAsset ? MeshAsset->GetName() : TEXT("None");
            const FString AnimClassName = AbilityMesh->GetAnimClass() ? AbilityMesh->GetAnimClass()->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[WatchAbility] FallbackMesh=%s AnimClass=%s"),
                *MeshAssetName,
                *AnimClassName);
        }
    }

    if (!PlayerCharacter->CanUseWatch())
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] CanUseWatch false. Local=%d"),
            ActorInfo->IsLocallyControlled() ? 1 : 0);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UAnimMontage* ActiveMontage = ResolveToggleWatchMontage(PlayerCharacter);
    if (!ActiveMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] ToggleWatchMontage is null."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] AnimInstance is null; cannot play montage."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (ActiveMontage->GetSkeleton() && AbilityMesh && AbilityMesh->GetSkeletalMeshAsset())
    {
        const USkeleton* MontageSkeleton = ActiveMontage->GetSkeleton();
        const USkeleton* MeshSkeleton = AbilityMesh->GetSkeletalMeshAsset()->GetSkeleton();
        const FString MontageSkeletonName = MontageSkeleton ? MontageSkeleton->GetName() : TEXT("None");
        const FString MeshSkeletonName = MeshSkeleton ? MeshSkeleton->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] MontageSkeleton=%s MeshSkeleton=%s Match=%d"),
            *MontageSkeletonName,
            *MeshSkeletonName,
            MontageSkeleton == MeshSkeleton ? 1 : 0);
    }

    for (const FSlotAnimationTrack& Track : ActiveMontage->SlotAnimTracks)
    {
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Montage slot: %s"), *Track.SlotName.ToString());
    }

    const bool bWasLookingAtWatch = PlayerCharacter->IsLookingAtWatch();
    if (ActorInfo->IsLocallyControlled())
    {
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Local toggle request. WasLooking=%d"), bWasLookingAtWatch ? 1 : 0);
        PlayerCharacter->HandleToggleWatchAbility();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Non-local montage play. WasLooking=%d"), bWasLookingAtWatch ? 1 : 0);
    }

    StartWatchMontage(ActiveMontage, bWasLookingAtWatch, false);
    BeginWaitForInput();
}

void UEMRGameplayAbility_ToggleWatch::HandleWatchMontageCompleted()
{
    UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Montage completed."));
    SetWatchSocketLookActive(false);
    ActiveToggleWatchMontage.Reset();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEMRGameplayAbility_ToggleWatch::HandleWatchMontageCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Montage cancelled/interrupted."));
    if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
    {
        if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
        {
            if (const UAnimMontage* ActiveMontage = ActiveToggleWatchMontage.Get())
            {
                if (AnimInstance->Montage_IsPlaying(ActiveMontage))
                {
                    UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Montage still playing after interrupt; ignoring cancel."));
                    return;
                }
            }
            else if ((ToggleWatchMontage && AnimInstance->Montage_IsPlaying(ToggleWatchMontage)) ||
                     (ToggleWatchMontageSeated && AnimInstance->Montage_IsPlaying(ToggleWatchMontageSeated)))
            {
                UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Montage still playing after interrupt; ignoring cancel."));
                return;
            }
        }
    }

    SetWatchSocketLookActive(false);
    ActiveToggleWatchMontage.Reset();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UEMRGameplayAbility_ToggleWatch::HandleInputPressed(float TimeWaited)
{
    UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Input pressed again after %.2f seconds."), TimeWaited);

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Input press ignored: ActorInfo null."));
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetAvatarActorFromActorInfo());
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Input press ignored: Avatar not player character."));
        return;
    }

    if (!PlayerCharacter->CanUseWatch())
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Input press ignored: CanUseWatch false."));
        return;
    }

    UAnimMontage* ActiveMontage = ResolveToggleWatchMontage(PlayerCharacter);
    if (!ActiveMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Input press ignored: montage missing."));
        return;
    }

    const bool bWasLookingAtWatch = PlayerCharacter->IsLookingAtWatch();
    if (ActorInfo->IsLocallyControlled())
    {
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] Local toggle request (input). WasLooking=%d"), bWasLookingAtWatch ? 1 : 0);
        PlayerCharacter->HandleToggleWatchAbility();
    }

    StartWatchMontage(ActiveMontage, bWasLookingAtWatch, true);
    BeginWaitForInput();
}

void UEMRGameplayAbility_ToggleWatch::BeginWaitForInput()
{
    UAbilityTask_WaitInputPress* Task = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
    if (!Task)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Failed to create WaitInputPress task."));
        return;
    }

    Task->OnPress.AddDynamic(this, &ThisClass::HandleInputPressed);
    Task->ReadyForActivation();
    InputPressTask = Task;
}

void UEMRGameplayAbility_ToggleWatch::SetWatchSocketLookActive(bool bEnabled)
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
    {
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetAvatarActorFromActorInfo());
    if (!PlayerCharacter || (!PlayerCharacter->IsLocallyControlled() && !PlayerCharacter->HasAuthority()))
    {
        return;
    }

    PlayerCharacter->SetWatchSocketLookActive(bEnabled);
    if (!PlayerCharacter->HasAuthority())
    {
        PlayerCharacter->ServerSetWatchSocketLookActive(bEnabled);
    }
}

UAnimMontage* UEMRGameplayAbility_ToggleWatch::ResolveToggleWatchMontage(const AEMRPlayerCharacter* PlayerCharacter) const
{
    if (ActiveToggleWatchMontage.IsValid())
    {
        return ActiveToggleWatchMontage.Get();
    }

    if (PlayerCharacter && PlayerCharacter->IsSeated())
    {
        if (ToggleWatchMontageSeated)
        {
            return ToggleWatchMontageSeated;
        }

        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Seated montage missing; falling back to ToggleWatchMontage."));
    }

    return ToggleWatchMontage;
}

void UEMRGameplayAbility_ToggleWatch::StartWatchMontage(UAnimMontage* MontageToPlay, bool bPreferReverse, bool bReuseCurrentPosition)
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] StartWatchMontage aborted: ActorInfo null."));
        return;
    }

    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    if (!AnimInstance || !MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] StartWatchMontage aborted: AnimInstance or montage missing."));
        return;
    }

    ActiveToggleWatchMontage = MontageToPlay;
    const bool bWasPlaying = AnimInstance->Montage_IsPlaying(MontageToPlay);
    const float CurrentPosition = bWasPlaying ? AnimInstance->Montage_GetPosition(MontageToPlay) : 0.f;
    const float CurrentPlayRate = bWasPlaying ? AnimInstance->Montage_GetPlayRate(MontageToPlay) : 0.f;
    bool bReverse = bPreferReverse;

    if (bReuseCurrentPosition && bWasPlaying)
    {
        SetWatchSocketLookActive(true);
        const float MontageLength = MontageToPlay->GetPlayLength();
        const float Remaining = bReverse ? CurrentPosition : FMath::Max(0.f, MontageLength - CurrentPosition);
        const float DesiredTime = FMath::Max(ToggleReturnTime, 0.05f);
        const float RawRate = Remaining / DesiredTime;
        const float ClampedRate = FMath::Clamp(RawRate, 1.f, MaxTogglePlayRate);
        const float TargetRate = bReverse ? -ClampedRate : ClampedRate;
        AnimInstance->Montage_SetPlayRate(MontageToPlay, TargetRate);
        UE_LOG(LogTemp, Log, TEXT("[WatchAbility] FlipPlayRate to %.2f at pos=%.2f remaining=%.2f desired=%.2f"),
            TargetRate, CurrentPosition, Remaining, DesiredTime);
        return;
    }

    if (ActiveMontageTask.IsValid())
    {
        ActiveMontageTask->EndTask();
        ActiveMontageTask.Reset();
    }

    if (bWasPlaying)
    {
        AnimInstance->Montage_Stop(0.f, MontageToPlay);
    }

    const float PlayRate = bReverse ? -1.f : 1.f;
    float StartTimeSeconds = bReverse ? MontageToPlay->GetPlayLength() : 0.f;
    if (bReuseCurrentPosition && bWasPlaying)
    {
        StartTimeSeconds = CurrentPosition;
    }

    UE_LOG(LogTemp, Log, TEXT("[WatchAbility] PlayMontage rate=%.2f start=%.2f length=%.2f wasPlaying=%d pos=%.2f reverse=%d prevRate=%.2f"),
        PlayRate, StartTimeSeconds, MontageToPlay->GetPlayLength(), bWasPlaying ? 1 : 0, CurrentPosition, bReverse ? 1 : 0, CurrentPlayRate);

    SetWatchSocketLookActive(true);
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, MontageToPlay, PlayRate, NAME_None, true, 1.f, StartTimeSeconds);
    if (!Task)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchAbility] Failed to create montage task."));
        return;
    }

    Task->OnCompleted.AddDynamic(this, &ThisClass::HandleWatchMontageCompleted);
    Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleWatchMontageCancelled);
    Task->OnCancelled.AddDynamic(this, &ThisClass::HandleWatchMontageCancelled);
    Task->ReadyForActivation();
    ActiveMontageTask = Task;
}
