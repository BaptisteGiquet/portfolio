#pragma once

#include "CoreMinimal.h"

#define EMR_LAB_LOG(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("[LabAnalyzer] " Format), ##__VA_ARGS__)
