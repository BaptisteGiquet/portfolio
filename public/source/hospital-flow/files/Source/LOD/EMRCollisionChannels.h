#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// Custom Trace Channels
// Interactable traces now use visibility and filter by the interactable interface.
#define ECC_Interactable ECC_Visibility
#define ECC_InteractableWidgets ECC_GameTraceChannel3
#define ECC_Patient ECC_GameTraceChannel2
