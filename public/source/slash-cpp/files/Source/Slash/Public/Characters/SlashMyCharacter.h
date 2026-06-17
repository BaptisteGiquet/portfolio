
#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "InputActionValue.h"
#include "CharacterTypes.h"
#include "Interfaces/PickupInterface.h"
#include "SlashMyCharacter.generated.h"


class USlashOverlay;
class ASlashHUD;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UGroomComponent;
class AItem;


UCLASS()
class SLASH_API ASlashMyCharacter : public ABaseCharacter, public IPickupInterface
{
	GENERATED_BODY()

public:
	ASlashMyCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void SetOverlappingItem(AItem* InItem) override;
	virtual void AddSouls(ASoul* InSoul) override;
	virtual void AddGold(ATreasure* InTreasure) override;
	
protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void BeginPlay() override;
	
	/* Callbacks for Inputs */
	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	virtual void Jump() override;
	void Equip(const FInputActionValue& Value);
	virtual void Attack() override;
	void Dodge();

	/* Combat */
	virtual void HandleDamage(float DamageAmount) override;
	void UpdateHUDHealth();
	virtual bool CanAttack() override;
	virtual void OnAttackEnd() override;
	virtual void OnDodgeEnd() override;
	void Disarm();
	void Arm();
	bool CanEquip();
	void EquipWeapon(AWeapon*& Weapon);
	bool CanUnequip();
	virtual void Die() override;
	bool HasEnoughStamina();
	bool IsUnoccupied();
	bool IsDodging();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat)
	bool bIsEquiped;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Player)
	APlayerController* PlayerController;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<ASlashHUD> SlashHUD;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<USlashOverlay> SlashOverlay;
	
	/* Player Inputs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* SlashCharacterMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;
	
private:
	void InitializeSlashOverlay();
	void AddMappingContext();

	
	void PlayEquipMontage(FName Section);
	
	UFUNCTION(BlueprintCallable)
	void OnEquipEnd();

	UFUNCTION(BlueprintCallable)
	void OnHitReactEnd();
	
	UFUNCTION(BlueprintCallable)
	void AttachMeshToSocket(USceneComponent* InParent, FName InSocketName);
	
	/* Character Components */
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, Category = "Hair")
	UGroomComponent* Hair;

	UPROPERTY(VisibleAnywhere, Category = "Hair")
	UGroomComponent* Eyebrows;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Interaction")
	AItem* OverlappingItem;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* EquipMontage;

	UPROPERTY(VisibleInstanceOnly, Category = "States")
	ECharacterState CharacterState = ECharacterState::ECS_Unoccupied;
	
	UPROPERTY(VisibleInstanceOnly, Category = "States")
	EEquipmentState EquipmentState = EEquipmentState::EES_Unequipped;



public:
	//inline functions are defined in header, compiler doesn't have to go search for it (to use mainly for setter and getter)
	UFUNCTION(BlueprintPure)
	FORCEINLINE AItem* GetOverlappingItem() const { return OverlappingItem; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE EEquipmentState GetEquipmentState() const { return EquipmentState; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
	
};
