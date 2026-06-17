#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRLobbyCharacterPreviewStage.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
struct FTimerHandle;

UCLASS()
class LOD_API AEMRLobbyCharacterPreviewStage : public AActor
{
    GENERATED_BODY()

public:
    AEMRLobbyCharacterPreviewStage();

    void SetCharacterIndex(int32 NewIndex);
    void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);
    void CapturePreview();
    void SetPreviewCaptureEnabled(bool bEnabled);
    void RequestPreviewCapture();
    UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }
    void ClearPreview();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<USkeletalMeshComponent> PreviewMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<USceneCaptureComponent2D> SceneCapture;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

private:
    void ApplyCharacterDefinition();
    void DestroyPreviewActor();

    int32 CharacterIndex = INDEX_NONE;

    bool bPreviewCaptureEnabled = false;
    bool bPreviousCaptureEveryFrame = false;
    bool bPreviousCaptureOnMovement = false;

    FTimerHandle DeferredCaptureHandle;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> PreviewActor;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Characters")
    float PreviewFOV = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Characters")
    float CameraDistanceMultiplier = 1.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Characters")
    FVector CameraOffset = FVector::ZeroVector;
};
