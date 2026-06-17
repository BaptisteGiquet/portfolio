


#include "UI/Frontend/Core/EMRFrontendCommonListView.h"

#include "Editor/WidgetCompilerLog.h"
#include "UI/Frontend/Data/EMRDataListEntryMapping.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"
#include "UI/Frontend/Data/EMRListDataObjectCollection.h"


UUserWidget& UEMRFrontendCommonListView::OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (IsDesignTime())
	{
		return Super::OnGenerateEntryWidgetInternal(Item, DesiredEntryClass, OwnerTable);
	}
	
	if (TSubclassOf<UEMRFrontendCommonListEntryWidgetBase> FoundWidgetClass = DataListEntryMapping->FindEntryWidgetClassByDataObject(CastChecked<UEMRListDataObjectBase>(Item)))
	{
		return GenerateTypedEntry<UEMRFrontendCommonListEntryWidgetBase>(FoundWidgetClass, OwnerTable);	
	}
	
	return Super::OnGenerateEntryWidgetInternal(Item, DesiredEntryClass, OwnerTable);
}


bool UEMRFrontendCommonListView::OnIsSelectableOrNavigableInternal(UObject* FirstSelectedItem)
{
	return !FirstSelectedItem->IsA<UEMRListDataObjectCollection>();
}


#if WITH_EDITOR
void UEMRFrontendCommonListView::ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);
	
	if (!DataListEntryMapping)
	{
		CompileLog.Error(FText::FromString(TEXT("The variable DataListEntryMapping has no valid data asset assigned ")
			+ GetClass()->GetName()
			+ TEXT(" need a valid data asset to function properly.")));
	}
}
#endif
