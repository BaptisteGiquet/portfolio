#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "EMRWidgetBlueprintDumpLibrary.generated.h"

UCLASS()
class LODEDITOR_API UEMRWidgetBlueprintDumpLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Returns a JSON payload with either { "ok": true, ... } or { "ok": false, "error": "..." }.
	UFUNCTION(BlueprintCallable, Category = "EMR|Editor|UMG")
	static FString ExportWidgetBlueprintTreeJson(const FString& WidgetBlueprintAssetPath, bool bPrettyPrint = true);

	// Saves the same JSON payload to disk. Relative paths are resolved against ProjectSavedDir.
	UFUNCTION(BlueprintCallable, Category = "EMR|Editor|UMG")
	static FString SaveWidgetBlueprintTreeJson(const FString& WidgetBlueprintAssetPath, const FString& OutputPath, bool bPrettyPrint = true);

	// One-shot migration helper: replace a slider/analog slider widget with a progress bar in a Widget Blueprint.
	UFUNCTION(BlueprintCallable, Category = "EMR|Editor|UMG")
	static FString ReplaceSliderWithProgressBar(
		const FString& WidgetBlueprintAssetPath,
		const FString& SliderWidgetName,
		const FString& ProgressBarWidgetName,
		bool bCompileBlueprint = true);
};
