
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MobaRenderActorTargetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UMobaRenderActorTargetInterface : public UInterface
{
	GENERATED_BODY()
};


class LOD_API IMobaRenderActorTargetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FVector GetCaptureLocalPosition() const = 0;
	virtual FRotator GetCaptureLocalRotation() const = 0;
};
