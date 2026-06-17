#include "UI/Settings/AGASSGameSettingListView.h"

#include "Components/ListView.h"
#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "GameSettingValueDiscrete.h"
#include "GameSettingValueScalar.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "UI/Settings/AGASSGameSettingKeyBind.h"
#include "UI/Settings/AGASSGameSettingWidgets.h"
#include "Widgets/GameSettingListEntry.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

UAGASSGameSettingListView::UAGASSGameSettingListView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get())
	{
		if (UClass* const NavigationEntryClass = FrontendSettings->GameSettingNavigationEntryWidgetClass.LoadSynchronous())
		{
			EntryWidgetClass = NavigationEntryClass;
		}
	}

	if (EntryWidgetClass == nullptr)
	{
		EntryWidgetClass = UAGASSGameSettingNavigationEntryWidget::StaticClass();
	}
}

#if WITH_EDITOR
void UAGASSGameSettingListView::ValidateCompiledDefaults(IWidgetCompilerLog& InCompileLog) const
{
	UListView::ValidateCompiledDefaults(InCompileLog);
}
#endif

UUserWidget& UAGASSGameSettingListView::OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable)
{
	UGameSetting* const SettingItem = Cast<UGameSetting>(Item);

	if (SettingItem == nullptr)
	{
		TSubclassOf<UGameSettingListEntryBase> PreviewEntryClass(DesiredEntryClass.Get());
		if (PreviewEntryClass == nullptr)
		{
			PreviewEntryClass = UAGASSGameSettingNavigationEntryWidget::StaticClass();
		}

		return GenerateTypedEntry<UGameSettingListEntryBase>(PreviewEntryClass, OwnerTable);
	}

	const TSubclassOf<UGameSettingListEntryBase> EntryClass = ResolveEntryWidgetClass(SettingItem, DesiredEntryClass);
	UGameSettingListEntryBase& EntryWidget = GenerateTypedEntry<UGameSettingListEntryBase>(EntryClass, OwnerTable);
	EntryWidget.SetSetting(SettingItem);
	return EntryWidget;
}

TSubclassOf<UGameSettingListEntryBase> UAGASSGameSettingListView::ResolveEntryWidgetClass(UGameSetting* SettingItem, TSubclassOf<UUserWidget> DesiredEntryClass) const
{
	const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get();

	if (Cast<UGameSettingValueScalar>(SettingItem) != nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingScalarEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingScalarEntryWidget::StaticClass());
	}

	if (Cast<UGameSettingValueDiscrete>(SettingItem) != nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingDiscreteEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingDiscreteEntryWidget::StaticClass());
	}

	if (Cast<UAGASSGameSettingKeyBind>(SettingItem) != nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingKeyBindEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingKeyBindEntryWidget::StaticClass());
	}

	if (Cast<UGameSettingAction>(SettingItem) != nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingActionEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingActionEntryWidget::StaticClass());
	}

	if (Cast<UGameSettingCollection>(SettingItem) != nullptr && Cast<UGameSettingCollectionPage>(SettingItem) == nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingSectionHeaderEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingSectionHeaderEntryWidget::StaticClass());
	}

	if (Cast<UGameSettingCollectionPage>(SettingItem) != nullptr)
	{
		if (FrontendSettings != nullptr)
		{
			if (UClass* const LoadedClass = FrontendSettings->GameSettingNavigationEntryWidgetClass.LoadSynchronous())
			{
				return TSubclassOf<UGameSettingListEntryBase>(LoadedClass);
			}
		}

		return TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingNavigationEntryWidget::StaticClass());
	}

	const TSubclassOf<UGameSettingListEntryBase> FallbackEntryClass(DesiredEntryClass);
	return FallbackEntryClass != nullptr
		? FallbackEntryClass
		: TSubclassOf<UGameSettingListEntryBase>(UAGASSGameSettingNavigationEntryWidget::StaticClass());
}
