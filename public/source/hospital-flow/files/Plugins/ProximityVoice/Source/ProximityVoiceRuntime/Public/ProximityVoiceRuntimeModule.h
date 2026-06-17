#pragma once

#include "Modules/ModuleManager.h"

class FProximityVoiceRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
