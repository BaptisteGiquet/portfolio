#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSEconomyDeveloperSettings.generated.h"

class UAGASSShopCatalog;
class UUserWidget;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Economy"))
class AGASSECONOMY_API UAGASSEconomyDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSEconomyDeveloperSettings();

	static const UAGASSEconomyDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Wallet", meta=(ClampMin="0"))
	int32 InitialSharedMoney;

	UPROPERTY(Config, EditAnywhere, Category="Shop")
	TSoftObjectPtr<UAGASSShopCatalog> DefaultShopCatalog;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	bool bShowTeamMoneyTestWidget = true;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftClassPtr<UUserWidget> TeamMoneyTestWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="UI", meta=(ClampMin="0"))
	int32 TeamMoneyTestWidgetZOrder = 20;
};
