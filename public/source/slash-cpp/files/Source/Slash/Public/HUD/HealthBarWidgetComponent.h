
#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "HealthBarWidgetComponent.generated.h"


class UHealthBarWidget;

UCLASS()
class SLASH_API UHealthBarWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void BeginPlay() override;
	
	UFUNCTION()
	void SetHealthPercent(float Percent);

private:
	UPROPERTY()
	UHealthBarWidget* HealthBarWidget;
};
