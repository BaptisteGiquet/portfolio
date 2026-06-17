
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaTreeWidget.generated.h"


class UCanvasPanelSlot;
class IMobaTreeNodeInterface;
class UCanvasPanel;

UCLASS()
class LOD_API UMobaTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void DrawTreeFromNode(const IMobaTreeNodeInterface* StartingNodeInterface);
	
private:
	void DrawAndCenterStream(bool bIsUpstream, const IMobaTreeNodeInterface* StartingNodeInterface, UUserWidget* CenterWidget, UCanvasPanelSlot* CenterWidgetSlot, TArray<UCanvasPanelSlot*>& OutStreamSlots);
	void DrawStream(bool bIsUpstream, const IMobaTreeNodeInterface* StartingNodeInterface, UUserWidget* StartingNodeWidget, UCanvasPanelSlot* StartingNodeSlot, int32 StartingNodeDepth, float& OutNextLeafXPosition, TArray<UCanvasPanelSlot*>& OutStreamSlots);
	void ClearTree();
	TPair<UUserWidget*, UCanvasPanelSlot*> CreateWidgetForNode(const IMobaTreeNodeInterface* InNode);
	void CreateConnection(UUserWidget* SourceWidget, UUserWidget* DestinationWidget);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Canvas_RootPanel = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D NodeSize = FVector2D(60.f , 60.f);

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D NodePadding = FVector2D(16.f , 30.f);

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FLinearColor ConnectionColor = FLinearColor::Gray;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	float ConnectionThickness = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D StartPortLocalCoord = FVector2D(0.5f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D DestinationPortLocalCoord = FVector2D(0.5f, 0.1f);

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D SourcePortDirection = FVector2D(0.f, 90.f);

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D DestinationPortDirection = FVector2D(0.f, 90.f);

	UPROPERTY()
	TObjectPtr<UObject> CurrentCenterObject = nullptr;
};
