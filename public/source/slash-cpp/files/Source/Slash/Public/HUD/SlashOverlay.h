
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlashOverlay.generated.h"


class UTextBlock;
class UProgressBar;

UCLASS()
class SLASH_API USlashOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealthBarPercent(float InPercent);
	void SetStaminaBarPercent(float InPercent);
	void SetGoldCountText(int32 InNum);
	void SetSoulsCountText(int32 InNum);
	
private:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar_ProgressBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar_ProgressBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GoldCount_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SoulsCount_Text;
};
