#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AGASSActivatableWidget.generated.h"

class UAGASSFrontendPrimaryLayoutWidget;
class UAGASSUIManagerSubsystem;

UCLASS(Abstract)
class AGASSFRONTEND_API UAGASSActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UAGASSActivatableWidget(const FObjectInitializer& ObjectInitializer);

	UAGASSUIManagerSubsystem* GetUIManager() const;
	UAGASSFrontendPrimaryLayoutWidget* GetPrimaryGameLayout() const;
};
