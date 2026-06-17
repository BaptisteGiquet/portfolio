
#pragma once

#include "CoreMinimal.h"
#include "Character/MobaCharacter.h"
#include "MobaPlayerCharacter.generated.h"

class UMobaInventoryComponent;
class UMobaAttributeSet_Hero;
class UMobaCameraAimComponent;
struct FInputActionValue;
struct FGameplayAbilityInputBinds;
enum class EAbilityInputID : uint8;

class UMobaAttributeSet;
class UMobaAbilitySystemComponent;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;

UCLASS()
class AMobaPlayerCharacter : public AMobaCharacter
{
	GENERATED_BODY()

public:
	AMobaPlayerCharacter();
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

protected:

	
private:
	UPROPERTY(VisibleAnywhere, Category = "Essential Variables")
	TWeakObjectPtr<APlayerController> CachedPlayerController;
	
	
	
	/**********************************************************************/
	/*                             Camera                                 */
	/**********************************************************************/

protected:
	
private:
	void InitializeCameraRig();

	

	
	UPROPERTY(VisibleDefaultsOnly, Category = "View")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "View")
	TObjectPtr<UCameraComponent> Camera = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "View")
	TObjectPtr<UMobaCameraAimComponent> CameraAimComponent = nullptr;


	
	/**********************************************************************/
	/*                             Inputs                                 */
	/**********************************************************************/

public:
	
protected:
	
private:
	void AddGameplayMappingContext() const;
	void HandleLookInput(const FInputActionValue& InputActionValue);
	void HandleMoveInput(const FInputActionValue& InputActionValue);
	
	void UpgradeAbilityLeaderDownInput(const FInputActionValue& InputActionValue);
	void UpgradeAbilityLeaderUpInput(const FInputActionValue& InputActionValue);

	void HandleUseInventoryItem(const FInputActionValue& InputActionValue);
	
	void HandleAbilityInput(const FInputActionValue& InputActionValue, const EAbilityInputID AbilityInputID);
	void SetInputEnabled(const bool bIsEnabled);
	
	bool bIsUpgradeAbilityLeaderDown = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> GameplayInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UseInventoryItemInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UpgradeAbilityLeaderInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<EAbilityInputID, TObjectPtr<const UInputAction>> GameplayAbilityInputActions;




	/**********************************************************************/
	/*                           GAMEPLAY ABILITY                         */
	/**********************************************************************/

private:
	UFUNCTION(Server, Reliable)
	void Server_SendGameplayEvent(FGameplayTag EventTag);
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability System")
	TObjectPtr<UMobaAttributeSet_Hero> HeroAttributeSet = nullptr;




	
	/**********************************************************************/
	/*                                 STUN                               */
	/**********************************************************************/

protected:
	virtual void OnStun() override;
	virtual void OnRecoverFromStun() override;
	
	

	

	/**********************************************************************/
	/*                             DEATH & RESPAWN                        */
	/**********************************************************************/

public:

protected:
	virtual void OnDead() override;
	virtual void OnRespawn() override;


	/**********************************************************************/
	/*                                UI                                  */
	/**********************************************************************/

private:
	UPROPERTY()
	TObjectPtr<UMobaInventoryComponent> PlayerInventoryComponent;
};

