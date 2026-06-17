#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"
#include "EMRCommonHubNightShiftListDataObject.generated.h"

class UEMRNightShiftDefinition;

UCLASS()
class LOD_API UEMRCommonHubNightShiftListDataObject : public UEMRListDataObjectBase
{
	GENERATED_BODY()

public:
	void Init(UEMRNightShiftDefinition* InDefinition, bool bInUnlocked, int32 InOfferIndex);

	UEMRNightShiftDefinition* GetDefinition() const { return Definition; }
	bool IsUnlocked() const { return bUnlocked; }
	int32 GetOfferIndex() const { return OfferIndex; }

protected:
	virtual void OnDataObjectInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEMRNightShiftDefinition> Definition = nullptr;

	UPROPERTY(Transient)
	bool bUnlocked = false;

	UPROPERTY(Transient)
	int32 OfferIndex = INDEX_NONE;
};
