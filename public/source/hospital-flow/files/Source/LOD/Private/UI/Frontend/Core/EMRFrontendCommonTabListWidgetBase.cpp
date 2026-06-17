#include "UI/Frontend/Core/EMRFrontendCommonTabListWidgetBase.h"

#include "Editor/WidgetCompilerLog.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"

void UEMRFrontendCommonTabListWidgetBase::RequestRegisterTab(const FName& InTabID, const FText& InTabDisplayName)
{
	RegisterTab(InTabID, TabButtonEntryWidgetClass, nullptr, -1);

	if (UEMRFrontendCommonButtonBase* FoundTabButton = Cast<UEMRFrontendCommonButtonBase>(GetTabButtonBaseByID(InTabID)))
	{
		FoundTabButton->SetButtonText(InTabDisplayName);
	}
}


#if WITH_EDITOR
void UEMRFrontendCommonTabListWidgetBase::ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	if (!TabButtonEntryWidgetClass)
	{
		CompileLog.Error(FText::FromString(TEXT("The variable TabButtonEntryWidgetClass has no valid entry specified. ") +
			GetClass()->GetName() + TEXT(" needs a valid entry widget class to function properly.")));
	}
}
#endif
