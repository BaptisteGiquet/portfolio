


#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "Widgets/CommonActivatableWidgetContainer.h"


void UEMRCommonPrimaryLayoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
	{
		UISubsystem->RegisterCreatedPrimaryLayoutWidget(this);
	}
}


UCommonActivatableWidgetContainerBase* UEMRCommonPrimaryLayoutWidget::FindWidgetStackByTag(const FGameplayTag& Intag) const
{
	checkf(RegisteredWidgetStackMap.Contains(Intag), TEXT("Can not find the widget stack by the tag %s"), *Intag.ToString());

	return RegisteredWidgetStackMap.FindRef(Intag);
}

void UEMRCommonPrimaryLayoutWidget::ClearAllWidgetStacks()
{
	for (const TPair<FGameplayTag, UCommonActivatableWidgetContainerBase*>& Entry : RegisteredWidgetStackMap)
	{
		if (Entry.Value)
		{
			Entry.Value->ClearWidgets();
		}
	}
}


void UEMRCommonPrimaryLayoutWidget::RegisterWidgetStack(FGameplayTag InStackTag, UCommonActivatableWidgetContainerBase* InStack)
{
	if (!IsDesignTime())
	{
		if (!RegisteredWidgetStackMap.Contains(InStackTag))
		{
			RegisteredWidgetStackMap.Add(InStackTag, InStack);
		}
	}
}
