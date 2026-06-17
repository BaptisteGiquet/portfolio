


#include "UI/Frontend/Data/EMRListDataObjectStringResolution.h"

#include "Kismet/KismetSystemLibrary.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "UI/Frontend/Utils/DebugHelper.h"


void UEMRListDataObjectStringResolution::InitResolutionValues()
{
	TArray<FIntPoint> AvailableResolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(AvailableResolutions);

	if (AvailableResolutions.IsEmpty())
	{
		MaximumAllowedResolution = ResolutionToValueString(FIntPoint(1920, 1080));
		SetDefaultValueFromString(MaximumAllowedResolution);
		return;
	}

	AvailableResolutions.Sort(
		[](const FIntPoint& A, const FIntPoint& B)->bool
		{
			return A.SizeSquared() < B.SizeSquared();
		});
	
	for (const FIntPoint& Resolution : AvailableResolutions)
	{
		AddDynamicOption(ResolutionToValueString(Resolution), ResolutionToDisplayText(Resolution));
	}
	
	MaximumAllowedResolution = ResolutionToValueString(AvailableResolutions.Last());
	
	SetDefaultValueFromString(MaximumAllowedResolution);
}


void UEMRListDataObjectStringResolution::OnDataObjectInitialized()
{
	Super::OnDataObjectInitialized();
	
	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		CurrentDisplayText = ResolutionToDisplayText(UEMRGameUserSettings::Get()->GetScreenResolution());
	}
}


FString UEMRListDataObjectStringResolution::ResolutionToValueString(const FIntPoint& InResolution) const
{
	return FString::Printf(TEXT("(X=%i,Y=%i)"), InResolution.X, InResolution.Y);
}


FText UEMRListDataObjectStringResolution::ResolutionToDisplayText(const FIntPoint& InResolution) const
{
	const FString DisplayString = FString::Printf(TEXT("%i x %i"), InResolution.X, InResolution.Y);
	
	return FText::FromString(DisplayString);
}
