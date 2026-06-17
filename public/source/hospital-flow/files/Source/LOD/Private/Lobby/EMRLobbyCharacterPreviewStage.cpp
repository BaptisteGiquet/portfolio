#include "Lobby/EMRLobbyCharacterPreviewStage.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkinnedMeshComponent.h"

#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/EMRLobbyGameState.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Math/RotationMatrix.h"
#include "TimerManager.h"
#include "Characters/Player/EMRPlayerCharacter.h"

AEMRLobbyCharacterPreviewStage::AEMRLobbyCharacterPreviewStage()
{
    bReplicates = false;
	
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
	
	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(GetRootComponent());

    SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCapture->SetupAttachment(GetRootComponent());
    SceneCapture->bCaptureEveryFrame = true;
    SceneCapture->bCaptureOnMovement = false;

    // Render the full scene (no ShowOnly/Hidden lists) so Blueprint lights affect the preview.
    SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;

    // Capture scene color so alpha encodes inverse opacity (for UI masking).
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

    SceneCapture->PostProcessSettings.bOverride_VignetteIntensity = true;
    SceneCapture->PostProcessSettings.VignetteIntensity = 0.0f;
}

void AEMRLobbyCharacterPreviewStage::SetCharacterIndex(int32 NewIndex)
{
    CharacterIndex = NewIndex;
    ApplyCharacterDefinition();
    RequestPreviewCapture();
}

void AEMRLobbyCharacterPreviewStage::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
    RenderTarget = InRenderTarget;
    if (SceneCapture)
    {
        SceneCapture->TextureTarget = RenderTarget;
    }
}

void AEMRLobbyCharacterPreviewStage::CapturePreview()
{
    if (SceneCapture)
    {
        SceneCapture->CaptureScene();
    }
}

void AEMRLobbyCharacterPreviewStage::SetPreviewCaptureEnabled(bool bEnabled)
{
    bPreviewCaptureEnabled = bEnabled;
    if (SceneCapture)
    {
        SceneCapture->bCaptureEveryFrame = bEnabled;
    }
}

void AEMRLobbyCharacterPreviewStage::RequestPreviewCapture()
{
    if (!SceneCapture)
    {
        return;
    }

    CapturePreview();
}

void AEMRLobbyCharacterPreviewStage::ClearPreview()
{
    DestroyPreviewActor();

    if (PreviewMesh)
    {
        PreviewMesh->SetSkeletalMesh(nullptr);
    }

    CharacterIndex = INDEX_NONE;

    if (SceneCapture)
    {
        SceneCapture->ClearHiddenComponents();
    }
}

void AEMRLobbyCharacterPreviewStage::ApplyCharacterDefinition()
{
    if (!PreviewMesh)
    {
        return;
    }

    if (SceneCapture && RenderTarget)
    {
        SceneCapture->TextureTarget = RenderTarget;
    }

    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    if (!Catalog || Catalog->Characters.IsEmpty())
    {
        static bool bWarnedMissingCatalog = false;
        if (!bWarnedMissingCatalog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LobbyCharacterPreview] Missing or empty lobby character catalog."));
            bWarnedMissingCatalog = true;
        }
        PreviewMesh->SetSkeletalMesh(nullptr);
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
        DestroyPreviewActor();
        PreviewMesh->SetSkeletalMesh(nullptr);
        return;
    }

    if (Definition->PlayerCharacterClass)
    {
        bool bSpawned = false;
        if (GetNetMode() != NM_DedicatedServer)
        {
            UClass* DesiredClass = Definition->PlayerCharacterClass;
            if (PreviewActor.IsValid() && PreviewActor->GetClass() == DesiredClass)
            {
                bSpawned = true;
            }
            else
            {
                DestroyPreviewActor();

                if (UWorld* World = GetWorld())
                {
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.Owner = this;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                    if (AActor* SpawnedActor = World->SpawnActor<AActor>(DesiredClass, GetActorTransform(), SpawnParams))
                    {
                        SpawnedActor->SetReplicates(false);
                        SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
                    	SpawnedActor->SetActorLocation(PreviewMesh->GetComponentLocation());
                    	SpawnedActor->SetActorRotation(PreviewMesh->GetComponentRotation());
                        SpawnedActor->SetActorRelativeScale3D(FVector::OneVector);
                        SpawnedActor->SetActorHiddenInGame(false);
                        SpawnedActor->SetActorEnableCollision(false);
                        SpawnedActor->SetActorTickEnabled(true);

                        PreviewActor = SpawnedActor;
                        bSpawned = true;
                    }
                }
            }
        }

        if (!bSpawned)
        {
            static bool bWarnedPreviewSpawnFail = false;
            if (!bWarnedPreviewSpawnFail)
            {
                UE_LOG(LogTemp, Warning, TEXT("[LobbyCharacterPreview] Failed to spawn preview character actor."));
                bWarnedPreviewSpawnFail = true;
            }
        }

        if (SceneCapture)
        {
	        SceneCapture->ClearHiddenComponents();
        }
        return;
    }

    DestroyPreviewActor();
    PreviewMesh->SetVisibility(true, true);
    if (SceneCapture)
    {
        SceneCapture->ClearHiddenComponents();
    }

    PreviewMesh->SetSkeletalMesh(Definition->SkeletalMesh);
    if (Definition->AnimClass)
    {
        PreviewMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        PreviewMesh->SetAnimInstanceClass(Definition->AnimClass);
    }
    else
    {
        PreviewMesh->SetAnimInstanceClass(nullptr);
    }
}


void AEMRLobbyCharacterPreviewStage::DestroyPreviewActor()
{
    if (AActor* SpawnedActor = PreviewActor.Get())
    {
        SpawnedActor->Destroy();
    }

    PreviewActor.Reset();
}
