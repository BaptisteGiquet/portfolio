#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AGASSFrontendPrototypeButtonBase.generated.h"

class UCommonTextBlock;

UCLASS(Abstract)
class AGASSFRONTEND_API UAGASSFrontendPrototypeButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AGASS Frontend")
	void SetButtonText(const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Label;
};
