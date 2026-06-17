#pragma once
#include "DrawDebugHelpers.h"

#define DRAW_SPHERE(Location, Color, Time) if (GetWorld()) DrawDebugSphere(GetWorld(), Location, 25.f, 24, Color, false, Time);
#define DRAW_SPHERE_SingleFrame(Location, Color) if (GetWorld()) DrawDebugSphere(GetWorld(), Location, 25.f, 24, Color, false, -1.f);
#define DRAW_LINE(StartLocation, EndLocation, Color, Time) if (GetWorld()) DrawDebugLine(GetWorld(), StartLocation, EndLocation, Color, true, -1.f, 0, Time);
#define DRAW_LINE_SingleFrame(StartLocation, EndLocation, Color) if (GetWorld()) DrawDebugLine(GetWorld(), StartLocation, EndLocation, Color, false, -1.f, 0, 1.f);
#define DRAW_Point(Location, Color, Time) if (GetWorld()) DrawDebugPoint(GetWorld(), Location, 15.f, Color, true);
#define DRAW_Point_SingleFrame(Location, Color) if (GetWorld()) DrawDebugPoint(GetWorld(), Location, 15.f, Color, false, -1.f);
#define DRAW_Vector(StartLocation, EndLocation, Color) DRAW_LINE(StartLocation, EndLocation, Color) DRAW_Point(EndLocation, Color);
#define DRAW_Vector_SingleFrame(StartLocation, EndLocation, Color) DRAW_LINE_SingleFrame(StartLocation, EndLocation, Color) DRAW_Point_SingleFrame(EndLocation, Color);

#define PRINT_To_Screen(...) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, FString::Printf(__VA_ARGS__));