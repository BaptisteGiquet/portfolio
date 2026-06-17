

#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectString.h"
#include "EMRListDataObjectStringResolution.generated.h"

/**
 * 
 */
UCLASS()
class LOD_API UEMRListDataObjectStringResolution : public UEMRListDataObjectString
{
	GENERATED_BODY()
	
public:
	void InitResolutionValues();
	
	FString GetMaximumAllowedResolution() const { return MaximumAllowedResolution; }
	
protected:
	virtual void OnDataObjectInitialized() override;
	
private:
	FString ResolutionToValueString(const FIntPoint& InResolution) const;
	FText ResolutionToDisplayText(const FIntPoint& InResolution) const;
	
	FString MaximumAllowedResolution;
};
