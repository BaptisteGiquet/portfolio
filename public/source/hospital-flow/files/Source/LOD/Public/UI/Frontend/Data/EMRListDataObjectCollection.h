
#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectBase.h"
#include "EMRListDataObjectCollection.generated.h"

/**
 * 
 */
UCLASS()
class LOD_API UEMRListDataObjectCollection : public UEMRListDataObjectBase
{
	GENERATED_BODY()

public:
	virtual TArray<UEMRListDataObjectBase*> GetAllChildListData() const override;
	virtual bool HasAnyChildListData() const override;

	void AddChildListData(UEMRListDataObjectBase* InChildListData);
	
private:
	UPROPERTY(Transient)
	TArray<UEMRListDataObjectBase*> ChildListDataArray;
};
