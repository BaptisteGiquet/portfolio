#pragma once

#include "CoreMinimal.h"
#include "Interaction/EMRBaseMachine.h"
#include "GameplayTagContainer.h"
#include "EMRReceptionMachine.generated.h"

class UWidgetComponent;
class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;
class AEMRPlayerCharacter;
class AActor;

UCLASS()
class LOD_API AEMRReceptionMachine : public AEMRBaseMachine
{
	GENERATED_BODY()

public:
    AEMRReceptionMachine();

    virtual void Tick(float DeltaSeconds) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
	UWidgetComponent* GetTriageWidgetComponent() const { return TriageWidgetComponent; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
    UWidgetComponent* GetTriageDetailPanelWidgetComponent() const { return TriageDetailPanelWidgetComponent; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
    UArrowComponent* GetSeatPoint() const { return SeatPoint; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
    UArrowComponent* GetExitPoint() const { return ExitPoint; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
    USceneComponent* GetSeatAttachComponent() const { return SeatAttachComponent; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine|Reception")
    AEMRPlayerCharacter* GetSeatedPlayer() const { return SeatedPlayer; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine|Reception")
    void SeatPlayer(AEMRPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine|Reception")
    void UnseatPlayer(AEMRPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine|Reception")
    void PlayValidationCueForNearbyPlayers(bool bSuccess, AActor* InstigatorActor);


protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<UWidgetComponent> TriageWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<UWidgetComponent> TriageDetailPanelWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<UStaticMeshComponent> SeatMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<UArrowComponent> SeatPoint = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<UArrowComponent> ExitPoint = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    TObjectPtr<USceneComponent> SeatAttachComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Machine|Reception")
    FGameplayTag SeatAnimationTag;



    UPROPERTY(ReplicatedUsing = "OnRep_SeatedPlayer")
    TObjectPtr<AEMRPlayerCharacter> SeatedPlayer = nullptr;

    UFUNCTION()
    void OnRep_SeatedPlayer();

private:
    void AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player);
    void DetachSeatMeshToOriginal();
    void UpdateSeatMeshYawFromPlayer(float DeltaSeconds);
    void ApplySeatMeshYawOffset();
    UFUNCTION()
    void OnRep_SeatMeshYawOffset();

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> SeatMeshOriginalAttachParent = nullptr;

    UPROPERTY(Transient)
    FTransform SeatMeshOriginalRelativeTransform;

    UPROPERTY(Transient)
    bool bSeatMeshAttachCached = false;

    UPROPERTY(Transient)
    bool bSeatMeshFollowYaw = false;

    UPROPERTY(Transient)
    FRotator SeatMeshBaseWorldRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient)
    float SeatMeshInitialPlayerYaw = 0.f;

    UPROPERTY(Transient)
    float SeatMeshInitialControlYaw = 0.f;

    UPROPERTY(ReplicatedUsing = "OnRep_SeatMeshYawOffset")
    float SeatMeshYawOffset = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine|Reception", meta = (ClampMin = "0.0"))
    float SeatYawInterpSpeed = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine|Reception|Audio")
    TObjectPtr<USoundBase> SeatEnterSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine|Reception|Audio")
    TObjectPtr<USoundBase> SeatExitSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine|Reception|Audio")
    TObjectPtr<USoundBase> ValidationSuccessSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine|Reception|Audio")
    TObjectPtr<USoundBase> ValidationFailureSound = nullptr;

    UPROPERTY(Transient)
    float SeatMeshYawOffsetSmoothed = 0.f;

    UPROPERTY(Transient)
    bool bSeatMeshYawInitialized = false;

};
