
#include "HUD/SlashOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void USlashOverlay::SetHealthBarPercent(float InPercent)
{
	if (HealthBar_ProgressBar)
	{
		HealthBar_ProgressBar->SetPercent(InPercent);	
	}
}

void USlashOverlay::SetStaminaBarPercent(float InPercent)
{
	if (StaminaBar_ProgressBar)
	{
		StaminaBar_ProgressBar->SetPercent(InPercent);	
	}
}

void USlashOverlay::SetGoldCountText(int32 InGold)
{
	if (GoldCount_Text)
	{
		FString GoldString = FString::FromInt(InGold);
		FText GoldText = FText::FromString(GoldString);
		GoldCount_Text->SetText(GoldText);
	}
}

void USlashOverlay::SetSoulsCountText(int32 InSouls)
{
	if (SoulsCount_Text)
	{
		FString SoulsString = FString::FromInt(InSouls);
		FText SoulsText = FText::FromString(SoulsString);
		SoulsCount_Text->SetText(SoulsText);
	}
}
