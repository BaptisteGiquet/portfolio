
#include "Components/AttributeComponent.h"


UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;	
	CurrentStamina = MaxStamina;
}

void UAttributeComponent::AddGold(int32 InGold)
{
	Gold += InGold;
}

void UAttributeComponent::AddSouls(int32 InSouls)
{
	Souls += InSouls;
}

bool UAttributeComponent::IsAlive()
{
	return CurrentHealth > 0.f;
}


void UAttributeComponent::ReceiveDamage(float Damage)
{
	float ClampedHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);
	CurrentHealth = ClampedHealth;
}


void UAttributeComponent::SetCurrentHealth(float InHealth)
{
 	CurrentHealth = FMath::Clamp(InHealth, 0.f, MaxHealth);
}

void UAttributeComponent::SetMaxHealth(const float InMaxHealth)
{
	MaxHealth = InMaxHealth;
}

float UAttributeComponent::GetHealthPercent()
{
	return CurrentHealth / MaxHealth;
}

void UAttributeComponent::SetCurrentStamina(const float InStamina)
{
	CurrentStamina = FMath::Clamp(InStamina, 0.f, MaxStamina);
}

void UAttributeComponent::SetMaxStamina(const float InMaxStamina)
{
	MaxStamina = InMaxStamina;
}

float UAttributeComponent::GetStaminaPercent()
{
	return CurrentStamina / MaxStamina;
}

void UAttributeComponent::AddStamina(const float StaminaAdded)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + StaminaAdded, 0.f, MaxStamina);
}

void UAttributeComponent::UseStamina(const float StaminaCost)
{
	CurrentStamina = FMath::Clamp(CurrentStamina - StaminaCost, 0.f, MaxStamina);
}

void UAttributeComponent::RegenStamina(float DeltaTime)
{
	return AddStamina(StaminaRegentRate * DeltaTime);
}



