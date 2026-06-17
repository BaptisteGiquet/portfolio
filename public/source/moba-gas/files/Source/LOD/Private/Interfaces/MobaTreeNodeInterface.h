
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MobaTreeNodeInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UMobaTreeNodeInterface : public UInterface
{
	GENERATED_BODY()
};


class LOD_API IMobaTreeNodeInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UUserWidget* CreateTreeNodeWidget() const = 0;
	virtual TArray<const IMobaTreeNodeInterface*> GetParentNodes() const = 0;
	virtual TArray<const IMobaTreeNodeInterface*> GetChildNodes() const = 0;
	virtual UObject* GetNodeObject() const = 0;
};
