#pragma once

#include "CoreMinimal.h"
#include "Widgets/GameSettingListView.h"
#include "AGASSGameSettingListView.generated.h"

class UGameSetting;
class UGameSettingListEntryBase;

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingListView : public UGameSettingListView
{
	GENERATED_BODY()

public:
	UAGASSGameSettingListView(const FObjectInitializer& ObjectInitializer);

protected:
#if WITH_EDITOR
	virtual void ValidateCompiledDefaults(IWidgetCompilerLog& InCompileLog) const override;
#endif

	virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable) override;

private:
	TSubclassOf<UGameSettingListEntryBase> ResolveEntryWidgetClass(UGameSetting* SettingItem, TSubclassOf<UUserWidget> DesiredEntryClass) const;
};
