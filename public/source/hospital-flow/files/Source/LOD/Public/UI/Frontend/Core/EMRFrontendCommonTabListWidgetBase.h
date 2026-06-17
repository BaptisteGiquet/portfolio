

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "EMRFrontendCommonTabListWidgetBase.generated.h"

class UEMRFrontendCommonButtonBase;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendCommonTabListWidgetBase : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	void RequestRegisterTab(const FName& InTabID, const FText& InTabDisplayName);

private:
	
#if WITH_EDITOR
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|TabList Settings", meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 10))
	int32 DebugEditorPreviewTabCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|TabList Settings", meta = (AllowPrivateAccess = true))
	TSubclassOf<UEMRFrontendCommonButtonBase> TabButtonEntryWidgetClass;
};
