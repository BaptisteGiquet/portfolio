

#include "MobaTreeWidget.h"

#include "MobaSplineWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Interfaces/MobaTreeNodeInterface.h"



void UMobaTreeWidget::DrawTreeFromNode(const IMobaTreeNodeInterface* StartingNodeInterface)
{
	if (!StartingNodeInterface || !Canvas_RootPanel) { return; }
	if (CurrentCenterObject == StartingNodeInterface->GetNodeObject()) { return; }

	ClearTree();
	CurrentCenterObject = StartingNodeInterface->GetNodeObject();
	
	// Create the center node (will be moved to origin later)
	const TPair<UUserWidget*, UCanvasPanelSlot*> WidgetPair = CreateWidgetForNode(StartingNodeInterface);
	UUserWidget* CenterWidget = WidgetPair.Key;
	UCanvasPanelSlot* CenterWidgetSlot = WidgetPair.Value;

	// Draw parent tree (upstream) and center it horizontally at X=0
	TArray<UCanvasPanelSlot*> UpstreamSlots;
	DrawAndCenterStream(true, StartingNodeInterface, CenterWidget, CenterWidgetSlot, UpstreamSlots);

	// Draw children tree (downstream) and center it horizontally at X=0
	TArray<UCanvasPanelSlot*> DownstreamSlots;
	DrawAndCenterStream(false, StartingNodeInterface, CenterWidget, CenterWidgetSlot, DownstreamSlots);

	// Move the center node to the origin
	CenterWidgetSlot->SetPosition(FVector2D::ZeroVector);
}


void UMobaTreeWidget::DrawAndCenterStream(bool bIsUpstream, const IMobaTreeNodeInterface* StartingNodeInterface, UUserWidget* CenterWidget, UCanvasPanelSlot* CenterWidgetSlot, TArray<UCanvasPanelSlot*>& OutStreamSlots)
{
	float NextLeafXPosition = 0.f;

	// Recursively draw the tree, positioning nodes from left to right
	DrawStream(bIsUpstream, StartingNodeInterface, CenterWidget, CenterWidgetSlot, 0, NextLeafXPosition, OutStreamSlots);

	const float StreamTreeWidth = NextLeafXPosition - (NodeSize.X + NodePadding.X);
	const float OffsetToCenterX = -StreamTreeWidth / 2.0f;

	// Apply the centering offset to all nodes in the tree
	for (UCanvasPanelSlot* StreamSlot : OutStreamSlots)
	{
		StreamSlot->SetPosition(StreamSlot->GetPosition() + FVector2D(OffsetToCenterX, 0.f));
	}
}


void UMobaTreeWidget::DrawStream(bool bIsUpstream, const IMobaTreeNodeInterface* StartingNodeInterface, UUserWidget* StartingNodeWidget, UCanvasPanelSlot* StartingNodeSlot, int32 StartingNodeDepth, float& OutNextLeafXPosition, TArray<UCanvasPanelSlot*>& OutStreamSlots)
{

	UE_LOG(LogTemp, Warning, TEXT("Drawing %s node: %s at depth %d"), bIsUpstream ? TEXT("upstream") : TEXT("downstream"), *StartingNodeInterface->GetNodeObject()->GetName(), StartingNodeDepth + (bIsUpstream ? -1 : 1));
	TArray<const IMobaTreeNodeInterface*> NextTreeNodeInterfaces = bIsUpstream ? StartingNodeInterface->GetParentNodes() : StartingNodeInterface->GetChildNodes();

	const float StartingNodePosY = (NodeSize.Y + NodePadding.Y) * StartingNodeDepth;

	// For Leaf node: position it at the next available X position
	if (NextTreeNodeInterfaces.Num() == 0)
	{
		StartingNodeSlot->SetPosition(FVector2D(OutNextLeafXPosition, StartingNodePosY));
		OutNextLeafXPosition += NodeSize.X + NodePadding.X;
		return;
	}

	// Non-leaf node: recursively position children/parents first, then center this node
	float NextNodePosXSum = 0.f;
	
	for (const IMobaTreeNodeInterface* NextTreeNodeInterface : NextTreeNodeInterfaces)
	{
		const TPair<UUserWidget*, UCanvasPanelSlot*> WidgetPair = CreateWidgetForNode(NextTreeNodeInterface);
		UUserWidget* NextWidget = WidgetPair.Key;
		UCanvasPanelSlot* NextWidgetSlot = WidgetPair.Value;
		
		if (!NextWidget || !NextWidgetSlot) { continue; }

		OutStreamSlots.Add(NextWidgetSlot);
		if (bIsUpstream)
		{
			CreateConnection(NextWidget, StartingNodeWidget);
		}
		else
		{
			CreateConnection(StartingNodeWidget, NextWidget);
		}

		
		DrawStream(bIsUpstream, NextTreeNodeInterface, NextWidget, NextWidgetSlot, StartingNodeDepth + (bIsUpstream ? -1 : 1), OutNextLeafXPosition, OutStreamSlots);
		NextNodePosXSum += NextWidgetSlot->GetPosition().X;
	}

	const float StartingNodePosX = NextNodePosXSum / NextTreeNodeInterfaces.Num();
	StartingNodeSlot->SetPosition(FVector2D(StartingNodePosX, StartingNodePosY));
}


void UMobaTreeWidget::ClearTree()
{
	Canvas_RootPanel->ClearChildren();
}


TPair<UUserWidget*, UCanvasPanelSlot*> UMobaTreeWidget::CreateWidgetForNode(const IMobaTreeNodeInterface* InNode)
{
	if (!InNode) { return TPair<UUserWidget*, UCanvasPanelSlot*>(nullptr, nullptr); }

	UUserWidget* NodeWidget = InNode->CreateTreeNodeWidget();
	if (!NodeWidget) { return TPair<UUserWidget*, UCanvasPanelSlot*>(nullptr, nullptr); }


	UCanvasPanelSlot* CanvasSlot = Canvas_RootPanel->AddChildToCanvas(NodeWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetSize(NodeSize);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetZOrder(1);
	}

	return TPair<UUserWidget*, UCanvasPanelSlot*>(NodeWidget, CanvasSlot);
}


void UMobaTreeWidget::CreateConnection(UUserWidget* SourceWidget, UUserWidget* DestinationWidget)
{
	if (!SourceWidget || !DestinationWidget) { return; }

	UMobaSplineWidget* Connection = CreateWidget<UMobaSplineWidget>(GetOwningPlayer());
	if (!Connection) { return; }

	UCanvasPanelSlot* ConnectionSlot = Canvas_RootPanel->AddChildToCanvas(Connection);
	if (ConnectionSlot)
	{
		ConnectionSlot->SetAnchors(FAnchors(0.f, 0.f));
		ConnectionSlot->SetAlignment(FVector2D::ZeroVector);
		ConnectionSlot->SetPosition(FVector2D::ZeroVector);
		ConnectionSlot->SetZOrder(0);
	}

	Connection->SetupSpline(SourceWidget, DestinationWidget, StartPortLocalCoord, DestinationPortLocalCoord, SourcePortDirection, DestinationPortDirection);
	Connection->SetSplineStyle(ConnectionColor, ConnectionThickness);
}
