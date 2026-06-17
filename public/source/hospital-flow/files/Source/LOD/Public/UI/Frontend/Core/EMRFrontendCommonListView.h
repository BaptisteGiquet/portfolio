

#pragma once

#include "CoreMinimal.h"
#include "CommonListView.h"
#include "EMRFrontendCommonListView.generated.h"

class UEMRDataListEntryMapping;
/**
 * 
 */
UCLASS()
class LOD_API UEMRFrontendCommonListView : public UCommonListView
{
	GENERATED_BODY()

protected:
	virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable) override;
	virtual bool OnIsSelectableOrNavigableInternal(UObject* FirstSelectedItem) override;
	
private:
#if WITH_EDITOR
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif
	
	UPROPERTY(EditAnywhere, Category = "EMR|List View Settings")
	TObjectPtr<UEMRDataListEntryMapping> DataListEntryMapping;
	
};
