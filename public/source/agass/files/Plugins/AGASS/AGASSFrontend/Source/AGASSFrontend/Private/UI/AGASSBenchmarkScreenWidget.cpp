#include "UI/AGASSBenchmarkScreenWidget.h"

#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSBenchmarkScreen, Log, All);

namespace
{
	const FName BenchmarkStringTableId(TEXT("/Game/AGASS/Localization/ST_01.ST_01"));

	FText GetBenchmarkText(const TCHAR* Key, const FText& Fallback)
	{
		const TArray<FString> AvailableKeys = UKismetStringTableLibrary::GetKeysFromStringTable(BenchmarkStringTableId);
		return AvailableKeys.Contains(Key) ? FText::FromStringTable(BenchmarkStringTableId, Key) : Fallback;
	}
}

UAGASSBenchmarkScreenWidget::UAGASSBenchmarkScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = false;
	CurrentBodyText = GetBenchmarkText(
		TEXT("BenchmarkBodyRunning"),
		NSLOCTEXT("AGASSBenchmark", "BenchmarkBodyRunningFallback", "AGASS is benchmarking this machine and applying settings for stable 60 FPS. This only changes local graphics settings."));
}

FText UAGASSBenchmarkScreenWidget::GetTitleText() const
{
	return GetBenchmarkText(
		TEXT("BenchmarkTitle"),
		NSLOCTEXT("AGASSBenchmark", "BenchmarkTitleFallback", "Optimizing Performance"));
}

void UAGASSBenchmarkScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveWidgetByName(Text_BenchmarkBody, TEXT("Text_BenchmarkBody"));
	SetBodyText(CurrentBodyText);
}

void UAGASSBenchmarkScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (!bBenchmarkStarted)
	{
		bBenchmarkStarted = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::StartBenchmark);
		}
		else
		{
			StartBenchmark();
		}
	}
}

bool UAGASSBenchmarkScreenWidget::NativeOnHandleBackAction()
{
	return true;
}

void UAGASSBenchmarkScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	Text_BenchmarkBody = AddBodyText(ContentBox, CurrentBodyText, 16, TEXT("Text_BenchmarkBody"));
}

void UAGASSBenchmarkScreenWidget::RefreshFromSessionState()
{
	if (Text_Title != nullptr)
	{
		Text_Title->SetText(GetTitleText());
	}

	if (Text_Status != nullptr)
	{
		Text_Status->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetBodyText(CurrentBodyText);
}

UWidget* UAGASSBenchmarkScreenWidget::ResolveInitialFocusTarget() const
{
	return nullptr;
}

#if WITH_EDITOR
void UAGASSBenchmarkScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Add(TEXT("Text_BenchmarkBody"));
}
#endif

void UAGASSBenchmarkScreenWidget::SetBodyText(const FText& InText)
{
	CurrentBodyText = InText;

	if (Text_BenchmarkBody != nullptr)
	{
		Text_BenchmarkBody->SetText(CurrentBodyText);
	}
}

void UAGASSBenchmarkScreenWidget::StartBenchmark()
{
	UAGASSSettingsLocal* LocalSettings = UAGASSSettingsLocal::Get();
	UE_LOG(LogAGASSBenchmarkScreen, Log, TEXT("Benchmark screen starting benchmark. LocalSettings=%s"), *GetNameSafe(LocalSettings));
	if (LocalSettings != nullptr)
	{
		LocalSettings->RunAutoBenchmark(true);
	}

	UE_LOG(LogAGASSBenchmarkScreen, Log, TEXT("Benchmark screen closing after benchmark run."));
	DeactivateWidget();
}
