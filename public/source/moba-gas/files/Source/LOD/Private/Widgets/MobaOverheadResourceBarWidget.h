
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaOverheadResourceBarWidget.generated.h"


class UMobaAbilitySystemComponent;
class UMobaHUDResourceBarWidget;
class UAbilitySystemComponent;

UCLASS()
class UMobaOverheadResourceBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent);

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UMobaHUDResourceBarWidget> HealthBar = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UMobaHUDResourceBarWidget> ManaBar = nullptr;
	
};
