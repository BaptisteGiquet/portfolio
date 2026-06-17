
#include "HUD/HealthBarWidgetComponent.h"

#include "Components/ProgressBar.h"
#include "HUD/HealthBarWidget.h"


void UHealthBarWidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	HealthBarWidget = Cast<UHealthBarWidget>(GetUserWidgetObject());	
}

void UHealthBarWidgetComponent::SetHealthPercent(float Percent)
{
	if (HealthBarWidget	&& HealthBarWidget->PB_Health)
	{
		HealthBarWidget->PB_Health->SetPercent(Percent);
	}
}
