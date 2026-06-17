

#include "EMRLoadingScreenSettings.h"
#include "Blueprint/UserWidget.h"


TSubclassOf<UUserWidget> UEMRLoadingScreenSettings::GetLoadingScreenWidgetClassChecked() const
{
	checkf(!LoadingScreenClass.IsNull(), TEXT("Forgot to assign a valid Widget Blueprint in the project settings as Loading Screen"));
	
	TSubclassOf<UUserWidget> LoadedLoadingScreenWidget = LoadingScreenClass.LoadSynchronous();
	
	return LoadedLoadingScreenWidget;
}
