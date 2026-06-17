
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "Interfaces/MobaRenderActorTargetInterface.h"
#include "MobaCharacter.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UWidgetComponent;
class UMobaOverheadResourceBarWidget;
class UMobaAttributeSet;
class UMobaAbilitySystemComponent;
class UAbilitySystemComponent;

UCLASS()
class AMobaCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface, public IMobaRenderActorTargetInterface
{
	GENERATED_BODY()

public:
	AMobaCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	void InitializeAbilitySystemComponent_Server();
	void InitializeAbilitySystemComponent_Client();
	bool IsLocallyControlledByPlayer() const;


protected:
	virtual void BeginPlay() override;
	
	TObjectPtr<UAnimInstance> GetAvatarAnimInstance();
	void RefreshAnimInstanceCache();

	void UpgradeAbilityWithInputID(const EAbilityInputID AbilityInputID);

private:
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Essential Variables")
	TWeakObjectPtr<UAnimInstance> CachedAvatarAnimInstance;

	FTransform MeshRelativeTransform;
	
	
	/**********************************************************************/
	/*                           GAMEPLAY ABILITY                         */
	/**********************************************************************/

	
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

	
private:
	void BindToGASChangeDelegate();
	void CancelAllAbilities();

	void OnMaxHealthUpdated(const FOnAttributeChangeData& HealthData);
	void OnMaxManaUpdated(const FOnAttributeChangeData& ManaData);
	void OnMoveSpeedUpdated(const FOnAttributeChangeData& MoveSpeedData);
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability System")
	TObjectPtr<UMobaAbilitySystemComponent> AbilitySystemComponent = nullptr;

	
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability System")
	TObjectPtr<UMobaAttributeSet> AttributeSet = nullptr;

	
	/**********************************************************************/
	/*                           User Interface                         */
	/**********************************************************************/

public:
	virtual FVector GetCaptureLocalPosition() const override;
	virtual FRotator GetCaptureLocalRotation() const override;

private:
	void ConfigureOverheadWidget();
	void UpdateOverheadResourceBarVisibility();
	void SetOverheadResourceBarWidgetEnabled(const bool bIsEnabled);


	UPROPERTY(EditDefaultsOnly, Category = "Capture")
	FVector HeroPortraitCaptureLocalPosition = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Capture")
	FRotator HeroPortraitCaptureLocalRotation = FRotator::ZeroRotator;

	
	UPROPERTY(VisibleDefaultsOnly, Category = "UI|Combat")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Combat")
	float OverheadVisibilityUpdateFrequency = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Combat")
	float OverheadVisibilityMaxRange = 5000.f;
	
	FTimerHandle OverheadVisibilityTimerHandle;


	
	/**********************************************************************/
	/*                               STUN                                 */
	/**********************************************************************/

protected:
	virtual void OnStun();
	virtual void OnRecoverFromStun();
	
	bool IsStun() { return bIsStun; }
	
private:
	void StunTagUpdated(const FGameplayTag Tag, const int32 StunTagCount);
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Stun")
	TObjectPtr<UAnimMontage> StunMontage;

	bool bIsStun = false;

	
	/**********************************************************************/
	/*                           DEATH & RESPAWN                          */
	/**********************************************************************/

protected:
	virtual void OnDead();
	virtual void OnRespawn();

	bool IsDead() { return bIsDead; }

private:
	void DeathTagUpdated(const FGameplayTag Tag, const int32 DeadTagCount);
	void StartDeathSequence();
	void StartRespawnSequence();
	void PlayDeathAnimation();
	void OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void SetRagdollEnabled(const bool bIsEnabled);

	bool bIsDead = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Death")
	TObjectPtr<UAnimMontage> DeathMontage;


	/**********************************************************************/
	/*                              TEAM                                  */
	/**********************************************************************/

public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	UFUNCTION()
	virtual void OnRep_TeamID();
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, VisibleAnywhere, Category = "Team")
	FGenericTeamId TeamID;




	
	/**********************************************************************/
	/*                                AI                                  */
	/**********************************************************************/

public:

protected:

private:
	void SetAIPerceptionStimuliSourceEnabled(const bool bIsEnabled);
	
	UPROPERTY()
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuliSourceComp;
};
