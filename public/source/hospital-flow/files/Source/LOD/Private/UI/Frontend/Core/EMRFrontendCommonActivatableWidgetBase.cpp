


#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"

#include "UI/Frontend/EMRFrontendPlayerController.h"


AEMRFrontendPlayerController* UEMRFrontendCommonActivatableWidgetBase::GetOwningFrontendPlayerController()
{
	if (!CachedOwningFrontendPC.IsValid())
	{
		CachedOwningFrontendPC = GetOwningPlayer<AEMRFrontendPlayerController>();
	}

	return CachedOwningFrontendPC.IsValid()? CachedOwningFrontendPC.Get(): nullptr;
}
