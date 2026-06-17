
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "MobaInventoryItemDragDropOp.generated.h"


class UMobaItemWidget;
class UMobaInventoryItemSlotWidget;

UCLASS()
class LOD_API UMobaInventoryItemDragDropOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	void SetDraggedItem(UMobaInventoryItemSlotWidget* DraggedItem);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<UMobaItemWidget> DragVisualClass;
};
