

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EMROptionsDataRegistry.generated.h"

class UEMRListDataObjectBase;
class UEMRListDataObjectCollection;
/**
 * 
 */
UCLASS()
class LOD_API UEMROptionsDataRegistry : public UObject
{
	GENERATED_BODY()

public:
	// Called by OptionsScreen after the Object of type UOptionsDataRegistry is created.
	void InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer);

	const TArray<UEMRListDataObjectCollection*>& GetRegisteredOptionsTabCollections() const { return RegisteredOptionsTabCollections; }
	
	TArray<UEMRListDataObjectBase*> GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const;

private:
	void InitGameplayCollectionTab();
	void InitAudioCollectionTab();
	void InitVideoCollectionTab();
	void InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer);
	
	void FindChildListDataRecursively(UEMRListDataObjectBase* InParentData, TArray<UEMRListDataObjectBase*>& OutFoundChildListData) const;

	UPROPERTY(Transient)
	TArray<UEMRListDataObjectCollection*> RegisteredOptionsTabCollections;
};
