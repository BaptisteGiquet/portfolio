#include "Lobby/EMRLobbyCharacterDisplayActor.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/ShapeComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
    void GatherAttachedActors(AActor* RootActor, TArray<AActor*>& OutActors)
    {
        if (!RootActor)
        {
            return;
        }

        TArray<AActor*> Stack;
        Stack.Add(RootActor);

        while (!Stack.IsEmpty())
        {
        	AActor* Current = Stack.Pop(EAllowShrinking::No);
            if (!Current || OutActors.Contains(Current))
            {
                continue;
            }

            OutActors.Add(Current);

            TArray<AActor*> Attached;
            Current->GetAttachedActors(Attached);
            for (AActor* Child : Attached)
            {
                if (Child && !OutActors.Contains(Child))
                {
                    Stack.Add(Child);
                }
            }
        }
    }
}
#include "Engine/World.h"
#include "Framework/EMRLobbyGameState.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Net/UnrealNetwork.h"
#include "Utils/EMREndSessionTrace.h"

AEMRLobbyCharacterDisplayActor::AEMRLobbyCharacterDisplayActor()
{
    bReplicates = true;
    bAlwaysRelevant = true;

    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEMRLobbyCharacterDisplayActor::SetCharacterIndex(int32 NewIndex)
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.SetCharacterIndex skip no authority actor=%s requested=%d"),
            *GetNameSafe(this),
            NewIndex);
        return;
    }

    if (CharacterIndex == NewIndex)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.SetCharacterIndex unchanged actor=%s index=%d hasSpawned=%d hasMesh=%d"),
            *GetNameSafe(this),
            CharacterIndex,
            SpawnedCharacterActor.IsValid() ? 1 : 0,
            (MeshComponent && MeshComponent->GetSkeletalMeshAsset() != nullptr) ? 1 : 0);
        // Ensure initial/default character data is still applied when index doesn't change.
        if (!SpawnedCharacterActor.IsValid()
            && MeshComponent
            && MeshComponent->GetSkeletalMeshAsset() == nullptr)
        {
            OnRep_CharacterIndex();
        }
        return;
    }

    CharacterIndex = NewIndex;
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.SetCharacterIndex actor=%s index=%d"),
        *GetNameSafe(this),
        CharacterIndex);
    OnRep_CharacterIndex();
}

void AEMRLobbyCharacterDisplayActor::BeginPlay()
{
    Super::BeginPlay();
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.BeginPlay actor=%s auth=%d netmode=%d"),
        *GetNameSafe(this),
        HasAuthority() ? 1 : 0,
        static_cast<int32>(GetNetMode()));
    ApplyCharacterDefinition();
}

void AEMRLobbyCharacterDisplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.EndPlay actor=%s reason=%d"),
        *GetNameSafe(this),
        static_cast<int32>(EndPlayReason));
    DestroyCharacterActor();
    Super::EndPlay(EndPlayReason);
}


void AEMRLobbyCharacterDisplayActor::OnRep_CharacterIndex()
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.OnRep_CharacterIndex actor=%s index=%d"),
        *GetNameSafe(this),
        CharacterIndex);
    ApplyCharacterDefinition();
}


void AEMRLobbyCharacterDisplayActor::ApplyCharacterDefinition()
{
    if (!MeshComponent)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition aborted actor=%s missing MeshComponent"),
            *GetNameSafe(this));
        return;
    }

    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    if (!Catalog || Catalog->Characters.IsEmpty())
    {
        static bool bWarnedMissingCatalog = false;
        if (!bWarnedMissingCatalog)
        {
            EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition missing or empty catalog"));
            bWarnedMissingCatalog = true;
        }
        MeshComponent->SetSkeletalMesh(nullptr);
        return;
    }

    int32 ResolvedIndex = CharacterIndex;
    if (!Catalog->Characters.IsValidIndex(ResolvedIndex))
    {
        ResolvedIndex = 0;
    }

    const FLobbyCharacterDefinition* Definition = Catalog->GetCharacterDefinition(ResolvedIndex);
    if (!Definition)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition missing definition actor=%s index=%d resolvedIndex=%d"),
            *GetNameSafe(this),
            CharacterIndex,
            ResolvedIndex);
        DestroyCharacterActor();
        MeshComponent->SetSkeletalMesh(nullptr);
        return;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition actor=%s index=%d resolvedIndex=%d hasClass=%d"),
        *GetNameSafe(this),
        CharacterIndex,
        ResolvedIndex,
        Definition->PlayerCharacterClass ? 1 : 0);

    if (Definition->PlayerCharacterClass)
    {
        if (GetNetMode() != NM_DedicatedServer)
        {
            UClass* DesiredClass = Definition->PlayerCharacterClass;
            if (!SpawnedCharacterActor.IsValid() || SpawnedCharacterActor->GetClass() != DesiredClass)
            {
                DestroyCharacterActor();

                if (UWorld* World = GetWorld())
                {
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.Owner = this;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                    if (AActor* SpawnedActor = World->SpawnActor<AActor>(DesiredClass, GetActorTransform(), SpawnParams))
                    {
                        SpawnedActor->SetReplicates(false);
                        SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
                        SpawnedActor->SetActorHiddenInGame(false);
                        SpawnedActor->SetActorEnableCollision(false);
                        SpawnedActor->SetActorTickEnabled(true);

                        SpawnedCharacterActor = SpawnedActor;
                        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition spawned actor=%s class=%s"),
                            *GetNameSafe(SpawnedActor),
                            *GetNameSafe(DesiredClass));
                    }
                    else
                    {
                        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.ApplyCharacterDefinition failed to spawn class=%s"),
                            *GetNameSafe(DesiredClass));
                    }
                }
            }
        }

        if (AActor* SpawnedActor = SpawnedCharacterActor.Get())
        {
            SpawnedActor->SetActorRelativeLocation(Definition->RelativeTransform.GetLocation());
            SpawnedActor->SetActorRelativeRotation(FRotator::ZeroRotator);
            SpawnedActor->SetActorRelativeScale3D(FVector::OneVector);
            SpawnedActor->AddActorLocalRotation(Definition->SlotRotationOffset);
        	SpawnedActor->SetActorHiddenInGame(false);
            //RefreshSpawnedActorVisibility();

            /*if (UWorld* SpawnWorld = GetWorld())
            {
                SpawnWorld->GetTimerManager().ClearTimer(VisibilityRefreshHandle);
                VisibilityRefreshRemaining = 10;
                SpawnWorld->GetTimerManager().SetTimer(VisibilityRefreshHandle, this, &ThisClass::RefreshSpawnedActorVisibility, 0.2f, true);
            }*/

            if (UCapsuleComponent* CapsuleComponent = SpawnedActor->FindComponentByClass<UCapsuleComponent>())
            {
                CapsuleComponent->SetHiddenInGame(true);
                CapsuleComponent->SetVisibility(false, false);
                CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }

        return;
    }

    DestroyCharacterActor();
    MeshComponent->SetVisibility(true, true);

    MeshComponent->SetSkeletalMesh(Definition->SkeletalMesh);
    if (Definition->AnimClass)
    {
        MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        MeshComponent->SetAnimInstanceClass(Definition->AnimClass);
    }
    else
    {
        MeshComponent->SetAnimInstanceClass(nullptr);
    }

    MeshComponent->SetRelativeTransform(Definition->RelativeTransform);
    MeshComponent->AddLocalRotation(Definition->SlotRotationOffset);
}

void AEMRLobbyCharacterDisplayActor::DestroyCharacterActor()
{
    if (AActor* SpawnedActor = SpawnedCharacterActor.Get())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyCharacterDisplay.DestroyCharacterActor destroying actor=%s"),
            *GetNameSafe(SpawnedActor));
        SpawnedActor->Destroy();
    }

    SpawnedCharacterActor.Reset();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(VisibilityRefreshHandle);
    }
}


void AEMRLobbyCharacterDisplayActor::RefreshSpawnedActorVisibility()
{
    AActor* SpawnedActor = SpawnedCharacterActor.Get();
    if (!SpawnedActor)
    {
        return;
    }

    SpawnedActor->SetActorHiddenInGame(false);

    TArray<AActor*> ActorHierarchy;
    GatherAttachedActors(SpawnedActor, ActorHierarchy);
    for (AActor* HierarchyActor : ActorHierarchy)
    {
        if (!HierarchyActor)
        {
            continue;
        }

        HierarchyActor->SetActorHiddenInGame(false);

        TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(HierarchyActor);
        for (UPrimitiveComponent* Component : PrimitiveComponents)
        {
            if (!Component)
            {
                continue;
            }
        	
            Component->SetOnlyOwnerSee(false);
            Component->SetOwnerNoSee(false);
            Component->SetComponentTickEnabled(true);
        	
        }
    }

    if (VisibilityRefreshRemaining > 0)
    {
        --VisibilityRefreshRemaining;
        if (VisibilityRefreshRemaining <= 0)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().ClearTimer(VisibilityRefreshHandle);
            }
        }
    }
}

void AEMRLobbyCharacterDisplayActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRLobbyCharacterDisplayActor, CharacterIndex);
}

