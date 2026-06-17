#include "Interfaces/EMRLoadingProcessInterface.h"

bool IEMRLoadingProcessInterface::ShouldShowLoadingScreen(UObject* TestObject, FString& OutReason)
{
	if (TestObject == nullptr)
	{
		return false;
	}

	if (IEMRLoadingProcessInterface* LoadingProcess = Cast<IEMRLoadingProcessInterface>(TestObject))
	{
		FString Reason;
		if (LoadingProcess->ShouldShowLoadingScreen(Reason))
		{
			OutReason = Reason;
			return true;
		}
	}

	return false;
}
