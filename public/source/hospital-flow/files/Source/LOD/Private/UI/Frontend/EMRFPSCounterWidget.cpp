#include "UI/Frontend/EMRFPSCounterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "DLSSLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "StreamlineLibraryDLSSG.h"
#include "UnrealClient.h"

extern ENGINE_API float GAverageFPS;
extern ENGINE_API float GAverageMS;

namespace EMRFPSCounterWidgetPrivate
{
	constexpr float FPSWidgetRefreshIntervalSeconds = 0.25f;
	constexpr float FpsSmoothingTimeConstantSeconds = 0.60f;
	constexpr float FrameMsSmoothingTimeConstantSeconds = 0.40f;

	bool IsEngineFeatureQueryReady()
	{
		return GEngine && GEngine->IsInitialized();
	}

	FString GetGraphicsRHIName()
	{
		const FString RHIName = FApp::GetGraphicsRHI();
		return RHIName.IsEmpty() ? TEXT("<unknown>") : RHIName;
	}

	template <typename TEnum>
	FString EnumValueToNameString(TEnum Value)
	{
		if (const UEnum* Enum = StaticEnum<TEnum>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Value));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Value));
	}

	FString BoolToOnOff(bool bValue)
	{
		return bValue ? TEXT("On") : TEXT("Off");
	}

	float ComputeEmaAlpha(float DeltaSeconds, float TimeConstantSeconds)
	{
		const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
		const float SafeTau = FMath::Max(0.001f, TimeConstantSeconds);
		return FMath::Clamp(1.0f - FMath::Exp(-SafeDelta / SafeTau), 0.0f, 1.0f);
	}
}

#define FPSWidgetRefreshIntervalSeconds EMRFPSCounterWidgetPrivate::FPSWidgetRefreshIntervalSeconds
#define FpsSmoothingTimeConstantSeconds EMRFPSCounterWidgetPrivate::FpsSmoothingTimeConstantSeconds
#define FrameMsSmoothingTimeConstantSeconds EMRFPSCounterWidgetPrivate::FrameMsSmoothingTimeConstantSeconds
#define IsEngineFeatureQueryReady EMRFPSCounterWidgetPrivate::IsEngineFeatureQueryReady
#define GetGraphicsRHIName EMRFPSCounterWidgetPrivate::GetGraphicsRHIName
#define EnumValueToNameString EMRFPSCounterWidgetPrivate::EnumValueToNameString
#define BoolToOnOff EMRFPSCounterWidgetPrivate::BoolToOnOff
#define ComputeEmaAlpha EMRFPSCounterWidgetPrivate::ComputeEmaAlpha

UEMRFPSCounterWidget::UEMRFPSCounterWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEMRFPSCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureWidgetHierarchy();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetAlignmentInViewport(FVector2D::ZeroVector);
	SetPositionInViewport(FVector2D(16.0f, 16.0f), false);
	SetDesiredSizeInViewport(FVector2D(640.0f, 180.0f));

	CachedPresentedSource = TEXT("Rendered");
	CachedFGStateNote.Reset();
	RefreshDisplayText();
}

void UEMRFPSCounterWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	SampleTelemetryOncePerEngineFrame();

	UIRefreshAccumulator += FMath::Max(0.0f, InDeltaTime);
	if (UIRefreshAccumulator >= FPSWidgetRefreshIntervalSeconds)
	{
		UIRefreshAccumulator = 0.0f;
		RefreshDisplayText();
	}
}

void UEMRFPSCounterWidget::EnsureWidgetHierarchy()
{
	if (!WidgetTree)
	{
		return;
	}

	// If a Widget Blueprint subclass already provides a root widget, respect that layout.
	const bool bHasDesignerRoot = WidgetTree->RootWidget != nullptr;

	if (!StatsTextBlock && !bHasDesignerRoot)
	{
		StatsTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FPSCounterText"));
		StatsTextBlock->SetAutoWrapText(false);
		StatsTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)));
		StatsTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		StatsTextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	}

	if (!RootBorder && !bHasDesignerRoot)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FPSCounterBorder"));
		RootBorder->SetPadding(FMargin(10.0f, 8.0f));
		RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.78f));
		if (StatsTextBlock)
		{
			RootBorder->SetContent(StatsTextBlock);
		}
	}

	if (!WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = RootBorder;
	}
}

void UEMRFPSCounterWidget::SampleTelemetryOncePerEngineFrame()
{
	if (LastSampledFrameCounter == GFrameCounter)
	{
		return;
	}

	LastSampledFrameCounter = GFrameCounter;

	const float AppDeltaSeconds = static_cast<float>(FApp::GetDeltaTime());
	const float SmoothingDeltaSeconds = (FMath::IsFinite(AppDeltaSeconds) && AppDeltaSeconds > 0.0f) ? AppDeltaSeconds : (1.0f / 60.0f);
	if (FMath::IsFinite(AppDeltaSeconds) && AppDeltaSeconds > KINDA_SMALL_NUMBER)
	{
		const float RenderedFrameMs = AppDeltaSeconds * 1000.0f;
		const float RenderedFPS = 1.0f / AppDeltaSeconds;
		const float FpsAlpha = ComputeEmaAlpha(AppDeltaSeconds, FpsSmoothingTimeConstantSeconds);
		const float FrameMsAlpha = ComputeEmaAlpha(AppDeltaSeconds, FrameMsSmoothingTimeConstantSeconds);

		if (SmoothedRenderedFPS <= 0.0f || !FMath::IsFinite(SmoothedRenderedFPS))
		{
			SmoothedRenderedFPS = RenderedFPS;
			SmoothedRenderedFrameMs = RenderedFrameMs;
		}
		else
		{
			SmoothedRenderedFPS = FMath::Lerp(SmoothedRenderedFPS, RenderedFPS, FpsAlpha);
			SmoothedRenderedFrameMs = FMath::Lerp(SmoothedRenderedFrameMs, RenderedFrameMs, FrameMsAlpha);
		}
	}

	CachedEngineAverageFPS = GAverageFPS;
	CachedEngineAverageMs = GAverageMS;

	CachedUnitFrameMs = 0.0f;
	CachedUnitGameMs = 0.0f;
	CachedUnitRenderMs = 0.0f;
	CachedUnitRHIMs = 0.0f;
	CachedUnitGPUMs = 0.0f;

	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			if (const FStatUnitData* StatUnitData = ViewportClient->GetStatUnitData())
			{
				CachedUnitFrameMs = StatUnitData->FrameTime;
				CachedUnitGameMs = StatUnitData->GameThreadTime;
				CachedUnitRenderMs = StatUnitData->RenderThreadTime;
				CachedUnitRHIMs = StatUnitData->RHITTime;
				CachedUnitGPUMs = StatUnitData->GPUFrameTime[0];
			}
		}
	}

	float RawPresentedFPS = SmoothedRenderedFPS;
	CachedDLSSGFramesPresented = 0;
	FString NewPresentedSource = TEXT("Rendered");
	CachedFGStateNote.Reset();

	const bool bFSRFGEnabled = ReadIntCVar(TEXT("r.FidelityFX.FI.Enabled"), 0) > 0;
	const bool bFSRFGUpdatesGlobalFPS = ReadIntCVar(TEXT("r.FidelityFX.FI.UpdateGlobalFrameTime"), 0) > 0;

	if (IsEngineFeatureQueryReady())
	{
		const EStreamlineFeatureSupport DLSSGSupport = UStreamlineLibraryDLSSG::QueryDLSSGSupport();
		const EStreamlineDLSSGMode DLSSGMode = UStreamlineLibraryDLSSG::GetDLSSGMode();
		const bool bDLSSGModeActive = DLSSGMode != EStreamlineDLSSGMode::Off;

		if (DLSSGSupport == EStreamlineFeatureSupport::Supported && bDLSSGModeActive)
		{
			float DLSSGFrameRateHz = 0.0f;
			int32 DLSSGFramesPresented = 0;
			UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(DLSSGFrameRateHz, DLSSGFramesPresented);

			if (FMath::IsFinite(DLSSGFrameRateHz) && DLSSGFrameRateHz > 0.0f)
			{
				RawPresentedFPS = DLSSGFrameRateHz;
				CachedDLSSGFramesPresented = DLSSGFramesPresented;
				NewPresentedSource = TEXT("DLSSG API");
			}
		}
	}

	if (bFSRFGEnabled && NewPresentedSource.Equals(TEXT("Rendered"), ESearchCase::CaseSensitive))
	{
		if (bFSRFGUpdatesGlobalFPS && CachedEngineAverageFPS > 0.0f)
		{
			RawPresentedFPS = CachedEngineAverageFPS;
			NewPresentedSource = TEXT("FSR FI + GAverageFPS");
		}
		else
		{
			CachedFGStateNote = TEXT("FSR FG is active but Unreal average FPS excludes generated frames unless r.FidelityFX.FI.UpdateGlobalFrameTime=1");
		}
	}

	const bool bResetPresentedSmoothing =
	SmoothedPresentedFPS <= 0.0f ||
	!FMath::IsFinite(SmoothedPresentedFPS) ||
	!CachedPresentedSource.Equals(NewPresentedSource, ESearchCase::CaseSensitive);

	if (bResetPresentedSmoothing)
	{
		SmoothedPresentedFPS = RawPresentedFPS;
	}
	else
	{
		const float PresentedAlpha = ComputeEmaAlpha(SmoothingDeltaSeconds, FpsSmoothingTimeConstantSeconds);
		SmoothedPresentedFPS = FMath::Lerp(SmoothedPresentedFPS, RawPresentedFPS, PresentedAlpha);
	}

	CachedPresentedFPS = SmoothedPresentedFPS;
	CachedPresentedSource = MoveTemp(NewPresentedSource);
}

void UEMRFPSCounterWidget::RefreshDisplayText()
{
	CurrentDisplayText = BuildDisplayText();

	if (StatsTextBlock)
	{
		StatsTextBlock->SetText(FText::FromString(CurrentDisplayText));
	}

	ReceiveStatsTextUpdated(CurrentDisplayText);
}

FString UEMRFPSCounterWidget::BuildDisplayText() const
{
	TStringBuilder<1024> Builder;

	const bool bPIE = GIsPlayInEditorWorld;
	const bool bEditor = GIsEditor;
	const bool bVSyncEnabled = ReadIntCVar(TEXT("r.VSync"), 0) > 0;
	const float MaxFPS = ReadFloatCVar(TEXT("t.MaxFPS"), 0.0f);

	Builder.Appendf(TEXT("FPS (Custom)  [%s%s]\n"),
		bPIE ? TEXT("PIE") : TEXT("Game"),
		bEditor ? TEXT(" | Editor") : TEXT(""));

	Builder.Appendf(TEXT("Base Rendered FPS:      %6.0f FPS  (%5.2f ms)\n"),
		SmoothedRenderedFPS,
		SmoothedRenderedFrameMs);

	Builder.Appendf(TEXT("Currently Displayed FPS: %6.0f FPS  [Source: %s"),
		CachedPresentedFPS,
		*CachedPresentedSource);

	if (CachedDLSSGFramesPresented > 0)
	{
		Builder.Appendf(TEXT(", FramesPresented=%d"), CachedDLSSGFramesPresented);
	}
	Builder.Append(TEXT("]\n"));

	Builder.Appendf(TEXT("Unit(ms):   Frame=%5.2f  GT=%5.2f  RT=%5.2f  RHI=%5.2f  GPU=%5.2f\n"),
		CachedUnitFrameMs,
		CachedUnitGameMs,
		CachedUnitRenderMs,
		CachedUnitRHIMs,
		CachedUnitGPUMs);

	Builder.Appendf(TEXT("Display:    VSync=%s  MaxFPS=%s\n"),
		*BoolToOnOff(bVSyncEnabled),
		MaxFPS > 0.0f ? *FString::Printf(TEXT("%.0f"), MaxFPS) : TEXT("Unlimited"));

	Builder.Appendf(TEXT("Upscaler:   %s\n"), *BuildUpscalerStateText());
	Builder.Appendf(TEXT("Frame Gen:  %s\n"), *BuildFrameGenerationStateText());

	if (!CachedFGStateNote.IsEmpty())
	{
		Builder.Appendf(TEXT("Note: %s\n"), *CachedFGStateNote);
	}

	return Builder.ToString();
}

FString UEMRFPSCounterWidget::BuildUpscalerStateText() const
{
	const int32 DLSSEnabled = ReadIntCVar(TEXT("r.NGX.DLSS.Enable"), 0);
	const int32 FSRUpscalerEnabled = ReadIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 0);
	const int32 FSRQualityMode = ReadIntCVar(TEXT("r.FidelityFX.FSR.QualityMode"), -1);
	const float ScreenPercentage = ReadFloatCVar(TEXT("r.ScreenPercentage"), 100.0f);
	const int32 TemporalAAUpscaler = ReadIntCVar(TEXT("r.TemporalAA.Upscaler"), -1);
	const FString GraphicsRHI = GetGraphicsRHIName();

	FString SelectedMode = TEXT("TSR/Engine");
	if (DLSSEnabled > 0)
	{
		SelectedMode = TEXT("DLSS");
	}
	else if (FSRUpscalerEnabled > 0)
	{
		SelectedMode = FString::Printf(TEXT("FSR (QMode=%d)"), FSRQualityMode);
	}

	FString DLSSModeString = TEXT("n/a");
	if (IsEngineFeatureQueryReady() && UDLSSLibrary::QueryDLSSSupport() == UDLSSSupport::Supported)
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		DLSSModeString = EnumValueToNameString(UDLSSLibrary::GetDLSSMode());
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	return FString::Printf(TEXT("%s | DLSS=%d (Mode=%s) | FSR=%d (Q=%d) | r.ScreenPercentage=%.1f | r.TemporalAA.Upscaler=%d | RHI=%s"),
		*SelectedMode,
		DLSSEnabled,
		*DLSSModeString,
		FSRUpscalerEnabled,
		FSRQualityMode,
		ScreenPercentage,
		TemporalAAUpscaler,
		*GraphicsRHI);
}

FString UEMRFPSCounterWidget::BuildFrameGenerationStateText() const
{
	const int32 FSRFGEnabled = ReadIntCVar(TEXT("r.FidelityFX.FI.Enabled"), 0);
	const int32 FSRFGUpdateGlobal = ReadIntCVar(TEXT("r.FidelityFX.FI.UpdateGlobalFrameTime"), 0);
	const int32 DLSSGEnableCVar = ReadIntCVar(TEXT("r.Streamline.DLSSG.Enable"), 0);
	const int32 DLSSGFramesToGenerate = ReadIntCVar(TEXT("r.Streamline.DLSSG.FramesToGenerate"), 0);
	const FString GraphicsRHI = GetGraphicsRHIName();

	FString DLSSGSupportString = TEXT("Unknown");
	FString DLSSGModeString = TEXT("Unknown");
	if (IsEngineFeatureQueryReady())
	{
		DLSSGSupportString = EnumValueToNameString(UStreamlineLibraryDLSSG::QueryDLSSGSupport());
		DLSSGModeString = EnumValueToNameString(UStreamlineLibraryDLSSG::GetDLSSGMode());
	}

	return FString::Printf(TEXT("DLSSG=%s (Support=%s, r.Enable=%d, FramesToGenerate=%d) | FSR_FI=%d (UpdateGlobalFPS=%d) | RHI=%s"),
		*DLSSGModeString,
		*DLSSGSupportString,
		DLSSGEnableCVar,
		DLSSGFramesToGenerate,
		FSRFGEnabled,
		FSRFGUpdateGlobal,
		*GraphicsRHI);
}

float UEMRFPSCounterWidget::ReadFloatCVar(const TCHAR* Name, float DefaultValue) const
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		return CVar->GetFloat();
	}

	return DefaultValue;
}

int32 UEMRFPSCounterWidget::ReadIntCVar(const TCHAR* Name, int32 DefaultValue) const
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		return CVar->GetInt();
	}

	return DefaultValue;
}

#undef ComputeEmaAlpha
#undef BoolToOnOff
#undef EnumValueToNameString
#undef GetGraphicsRHIName
#undef IsEngineFeatureQueryReady
#undef FrameMsSmoothingTimeConstantSeconds
#undef FpsSmoothingTimeConstantSeconds
#undef FPSWidgetRefreshIntervalSeconds
