
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaSplineWidget.generated.h"


UCLASS()
class LOD_API UMobaSplineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupSpline(
		UUserWidget* InStartWidget,
		UUserWidget* InEndWidget,
		const FVector2D& InStartPortLocalCoord,
		const FVector2D& InEndPortLocalCoord,
		const FVector2D& InStartPortDirection,
		const FVector2D& InEndPortDirection
		);

	void SetSplineStyle(const FLinearColor& InColor, const float InThickness);

	
protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	
private:
	UPROPERTY()
	TObjectPtr<UUserWidget> StartWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> EndWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Spline")
	FVector2D StartPortDirection = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Spline")
	FVector2D EndPortDirection = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Spline")
	FVector2D TestStartPos = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Spline")
	FVector2D TestEndPos = FVector2D(100.f, 100.f);
	
	FVector2D StartPortLocalCoord = FVector2D::ZeroVector;
	FVector2D EndPortLocalCoord = FVector2D::ZeroVector;

	
	
	UPROPERTY(EditAnywhere, Category = "Spline")
	FLinearColor SplineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Spline")
	float SplineThickness = 1.f;
};
