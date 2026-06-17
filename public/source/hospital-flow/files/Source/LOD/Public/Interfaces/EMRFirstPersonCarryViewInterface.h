#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EMRFirstPersonCarryViewInterface.generated.h"

class UUserWidget;

USTRUCT(BlueprintType)
struct FEMRFirstPersonCarryWidgetSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Carry|FirstPerson")
	TSubclassOf<UUserWidget> WidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Carry|FirstPerson")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Carry|FirstPerson")
	bool bDrawAtDesiredSize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Carry|FirstPerson")
	bool bShouldShow = true;
};

UINTERFACE(BlueprintType)
class UEMRFirstPersonCarryViewInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRFirstPersonCarryViewInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|FirstPerson")
	bool GetFirstPersonCarryWidgetSettings(FEMRFirstPersonCarryWidgetSettings& OutSettings) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|FirstPerson")
	void UpdateFirstPersonCarryWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|FirstPerson")
	void ResetFirstPersonCarryWidget(UUserWidget* Widget);
};
