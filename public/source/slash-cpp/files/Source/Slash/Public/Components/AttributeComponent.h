#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SLASH_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void ReceiveDamage(float Damage);

	UFUNCTION()
	bool IsAlive();

	void AddGold(int32 InGold);
	void AddSouls(int32 InSouls);

	UFUNCTION(BlueprintPure)
	float GetHealthPercent();

	UFUNCTION(BlueprintCallable)
	void SetCurrentHealth(const float InHealth);

	UFUNCTION(BlueprintCallable)
	void SetMaxHealth(const float InMaxHealth);

	UFUNCTION(BlueprintCallable)
	void SetCurrentStamina(const float InStamina);

	UFUNCTION(BlueprintCallable)
	void SetMaxStamina(const float InMaxStamina);

	UFUNCTION(BlueprintPure)
	float GetStaminaPercent();
	
	void AddStamina(const float StaminaAdded);

	void UseStamina(const float StaminaCost);

	void RegenStamina(float DeltaTime);
	
protected:
	virtual void BeginPlay() override;

	
private:
	UPROPERTY(VisibleAnywhere, Category = "Actor Attributes" , BlueprintGetter = GetCurrentHealth, BlueprintSetter = SetCurrentHealth)
	float CurrentHealth;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes", BlueprintGetter = GetMaxHealth, BlueprintSetter = SetMaxHealth)
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, Category = "Actor Attributes" , BlueprintGetter = GetCurrentStamina)
	float CurrentStamina;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes", BlueprintGetter = GetMaxStamina, BlueprintSetter = SetMaxStamina)
	float MaxStamina;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes", BlueprintGetter = GetDodgeStaminaCost)
	float DodgeStaminaCost = 20.f;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float StaminaRegentRate = 8.f;
	
	UPROPERTY(EditAnywhere, Category = "Actor Attributes", BlueprintGetter = GetGold)
	int32 Gold;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes", BlueprintGetter = GetSouls)
	int32 Souls;

public:
	UFUNCTION(BlueprintPure)
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure)
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure)
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure)
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure)
	float GetDodgeStaminaCost() const { return DodgeStaminaCost; }
	
	UFUNCTION(BlueprintPure)
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure)
	int32 GetSouls() const { return Souls; }
	
};
