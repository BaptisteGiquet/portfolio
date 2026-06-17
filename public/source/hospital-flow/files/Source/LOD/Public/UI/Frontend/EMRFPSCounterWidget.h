#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRFPSCounterWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(Blueprintable)
class LOD_API UEMRFPSCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEMRFPSCounterWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Stats")
	void ReceiveStatsTextUpdated(const FString& DisplayText);

	UFUNCTION(BlueprintPure, Category = "EMR|Stats")
	FString GetCurrentStatsDisplayText() const { return CurrentDisplayText; }

private:
	void EnsureWidgetHierarchy();
	void SampleTelemetryOncePerEngineFrame();
	void RefreshDisplayText();

	FString BuildDisplayText() const;
	FString BuildUpscalerStateText() const;
	FString BuildFrameGenerationStateText() const;

	float ReadFloatCVar(const TCHAR* Name, float DefaultValue = 0.0f) const;
	int32 ReadIntCVar(const TCHAR* Name, int32 DefaultValue = 0) const;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsTextBlock = nullptr;

	UPROPERTY(Transient)
	FString CurrentDisplayText;

	UPROPERTY(Transient)
	uint64 LastSampledFrameCounter = MAX_uint64;

	UPROPERTY(Transient)
	float SmoothedRenderedFPS = 0.0f;

	UPROPERTY(Transient)
	float SmoothedRenderedFrameMs = 0.0f;

	UPROPERTY(Transient)
	float CachedEngineAverageFPS = 0.0f;

	UPROPERTY(Transient)
	float CachedEngineAverageMs = 0.0f;

	UPROPERTY(Transient)
	float CachedUnitFrameMs = 0.0f;

	UPROPERTY(Transient)
	float CachedUnitGameMs = 0.0f;

	UPROPERTY(Transient)
	float CachedUnitRenderMs = 0.0f;

	UPROPERTY(Transient)
	float CachedUnitRHIMs = 0.0f;

	UPROPERTY(Transient)
	float CachedUnitGPUMs = 0.0f;

	UPROPERTY(Transient)
	float CachedPresentedFPS = 0.0f;

	UPROPERTY(Transient)
	float SmoothedPresentedFPS = 0.0f;

	UPROPERTY(Transient)
	int32 CachedDLSSGFramesPresented = 0;

	UPROPERTY(Transient)
	FString CachedPresentedSource;

	UPROPERTY(Transient)
	FString CachedFGStateNote;

	UPROPERTY(Transient)
	float UIRefreshAccumulator = 0.0f;
};
