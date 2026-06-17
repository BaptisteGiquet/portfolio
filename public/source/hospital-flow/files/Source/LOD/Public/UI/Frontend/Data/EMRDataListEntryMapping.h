#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EMRDataListEntryMapping.generated.h"

class UEMRFrontendCommonListEntryWidgetBase;
class UEMRListDataObjectBase;
/**
 * 
 */
UCLASS()
class LOD_API UEMRDataListEntryMapping : public UDataAsset
{
	GENERATED_BODY()
	
public:
	TSubclassOf<UEMRFrontendCommonListEntryWidgetBase> FindEntryWidgetClassByDataObject(UEMRListDataObjectBase* InDataObject) const;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TMap<TSubclassOf<UEMRListDataObjectBase>, TSubclassOf<UEMRFrontendCommonListEntryWidgetBase>> DataObjectListEntryMap;
};
