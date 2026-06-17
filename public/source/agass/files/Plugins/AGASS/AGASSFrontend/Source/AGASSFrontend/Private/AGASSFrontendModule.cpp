#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FAGASSFrontendModule : public IModuleInterface
{
};

IMPLEMENT_MODULE(FAGASSFrontendModule, AGASSFrontend)
