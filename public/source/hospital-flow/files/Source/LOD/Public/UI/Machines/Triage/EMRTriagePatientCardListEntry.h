#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "EMRTriagePatientCardListEntry.generated.h"

class UEMRTriagePatientCard;
class UEMRTriagePatientCardCommonButton;
class UAnalogSlider;
class UCommonTextBlock;
class UCommonLazyImage;
class UBorder;
class UEMRTriagePatientCardObject;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRTriagePatientCardListEntry : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeDestruct() override;

	void ResetEntry();
	
private:
	void ClearEntry();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRTriagePatientCard> Widget_TriagePatientCard;

	UPROPERTY()
	TObjectPtr<UEMRTriagePatientCardObject> CardObject;
	
};
