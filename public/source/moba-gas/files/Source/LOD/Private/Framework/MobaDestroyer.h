
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MobaDestroyer.generated.h"

enum class EMobaTeam : uint8;
class UCameraComponent;
class AAIController;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoalReachedDelegate, AActor* /* ViewTarget */, EMobaTeam /* WinningTeam */)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTeamUnitCountUpdatedDelegate, int32 /* TeamOneUnitCount */, int32 /* TeamTwoUnitCount */)


UCLASS()
class LOD_API AMobaDestroyer : public ACharacter
{
	GENERATED_BODY()

public:
	AMobaDestroyer();

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	float GetDestroyerProgress() const;

	FOnGoalReachedDelegate OnGoalReached;
	FOnTeamUnitCountUpdatedDelegate OnTeamUnitCountUpdated;
	
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

private:
	UFUNCTION()
	void OnUnitEnteredDetectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnUnitLeftDetectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_CoreToCapture();

	void UpdateTeamDominance();
	void UpdateDestroyerGoal();
	void GoalReached(const EMobaTeam WinningTeam);
	void CaptureCore();
	void OnExpandFinished();
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	TObjectPtr<UAnimMontage> ExpandMontage;

	UPROPERTY(EditAnywhere, Category = "Movement")
	TObjectPtr<UAnimMontage> CaptureMontage;
	
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<AActor> TeamOneGoal = nullptr;

	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<AActor> TeamTwoGoal = nullptr;

	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<AActor> TeamOneCore = nullptr;

	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<AActor> TeamTwoCore = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_CoreToCapture)
	TObjectPtr<AActor> CoreToCapture = nullptr;

	UPROPERTY(Replicated)
	float CoreCaptureSpeed = 0.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxMovementSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = "Detection")
	float UnitDetectionRadius = 1000.f;
	
	
	int32 TeamOneUnitCount = 0;
	int32 TeamTwoUnitCount = 0;

	float TeamDominance = 0.f;

	float DistanceBetweenTeams = 0.f;

	
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	TObjectPtr<USphereComponent> UnitDetectionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	TObjectPtr<UDecalComponent> GroundDecalComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	TObjectPtr<UCameraComponent> ViewCameraComponent;
	
	UPROPERTY()
	TObjectPtr<AAIController> OwnerAIController;
};
