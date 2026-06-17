//Parent class of WPB_HealhBar
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"


class UProgressBar;

UCLASS()
class SLASH_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_Health;
	
};
