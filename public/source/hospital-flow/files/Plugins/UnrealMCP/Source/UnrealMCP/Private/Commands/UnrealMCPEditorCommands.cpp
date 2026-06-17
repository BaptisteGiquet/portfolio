#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/World.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "SEditorViewport.h"

namespace
{
    TArray<TSharedPtr<FJsonValue>> VectorToJsonArray(const FVector& VectorValue)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        Result.Add(MakeShared<FJsonValueNumber>(VectorValue.X));
        Result.Add(MakeShared<FJsonValueNumber>(VectorValue.Y));
        Result.Add(MakeShared<FJsonValueNumber>(VectorValue.Z));
        return Result;
    }

    TArray<TSharedPtr<FJsonValue>> RotatorToJsonArray(const FRotator& RotatorValue)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        Result.Add(MakeShared<FJsonValueNumber>(RotatorValue.Pitch));
        Result.Add(MakeShared<FJsonValueNumber>(RotatorValue.Yaw));
        Result.Add(MakeShared<FJsonValueNumber>(RotatorValue.Roll));
        return Result;
    }

    TArray<TSharedPtr<FJsonValue>> NameArrayToJson(const TArray<FName>& Names)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        for (const FName& Name : Names)
        {
            Result.Add(MakeShared<FJsonValueString>(Name.ToString()));
        }
        return Result;
    }

    FString GetEnumValueName(const UEnum* EnumType, int64 Value)
    {
        if (!EnumType)
        {
            return FString::FromInt(static_cast<int32>(Value));
        }

        const FString Name = EnumType->GetNameStringByValue(Value);
        return Name.IsEmpty() ? FString::FromInt(static_cast<int32>(Value)) : Name;
    }

    AActor* FindActorByQuery(const FString& ActorQuery, FString& OutErrorMessage)
    {
        if (ActorQuery.IsEmpty())
        {
            OutErrorMessage = TEXT("Actor query is empty");
            return nullptr;
        }

        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);

        TArray<AActor*> ExactMatches;
        TArray<AActor*> PartialMatches;

        for (AActor* Actor : AllActors)
        {
            if (!Actor)
            {
                continue;
            }

            const FString Name = Actor->GetName();
            const FString Path = Actor->GetPathName();
#if WITH_EDITOR
            const FString Label = Actor->GetActorLabel(false);
#else
            const FString Label;
#endif

            if (Name == ActorQuery || Path == ActorQuery || Label == ActorQuery)
            {
                ExactMatches.Add(Actor);
                continue;
            }

            if (Name.Equals(ActorQuery, ESearchCase::IgnoreCase) ||
                Path.Equals(ActorQuery, ESearchCase::IgnoreCase) ||
                Label.Equals(ActorQuery, ESearchCase::IgnoreCase))
            {
                ExactMatches.Add(Actor);
                continue;
            }

            if (Name.Contains(ActorQuery, ESearchCase::IgnoreCase) ||
                ActorQuery.Contains(Name, ESearchCase::IgnoreCase) ||
                Label.Contains(ActorQuery, ESearchCase::IgnoreCase))
            {
                PartialMatches.Add(Actor);
            }
        }

        const TArray<AActor*>& Candidates = ExactMatches.Num() > 0 ? ExactMatches : PartialMatches;
        if (Candidates.Num() == 1)
        {
            return Candidates[0];
        }

        if (Candidates.Num() > 1)
        {
            FString CandidateList;
            for (AActor* Candidate : Candidates)
            {
                if (!CandidateList.IsEmpty())
                {
                    CandidateList += TEXT(", ");
                }
                CandidateList += Candidate->GetName();
            }

            OutErrorMessage = FString::Printf(TEXT("Actor query '%s' is ambiguous. Matches: %s"), *ActorQuery, *CandidateList);
            return nullptr;
        }

        OutErrorMessage = FString::Printf(TEXT("Actor not found: %s"), *ActorQuery);
        return nullptr;
    }

    void MarkObjectEdited(UObject* TargetObject)
    {
#if WITH_EDITOR
        if (!TargetObject)
        {
            return;
        }

        TargetObject->Modify();

        if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetObject))
        {
            PrimitiveComponent->MarkRenderStateDirty();
        }

        if (UActorComponent* Component = Cast<UActorComponent>(TargetObject))
        {
            Component->PostEditChange();
            Component->MarkPackageDirty();

            if (AActor* Owner = Component->GetOwner())
            {
                Owner->Modify();
                Owner->PostEditChange();
                Owner->MarkPackageDirty();
            }
            return;
        }

        if (AActor* Actor = Cast<AActor>(TargetObject))
        {
            Actor->PostEditChange();
            Actor->MarkPackageDirty();
            return;
        }

        TargetObject->MarkPackageDirty();
#endif
    }

    struct FActorFullInfoStats
    {
        int32 PrimitiveComponentCount = 0;
        int32 VisiblePrimitiveCount = 0;
        int32 ShadowCastingComponentCount = 0;
        int32 MovableShadowCasterCount = 0;
        int32 TotalMaterialSlots = 0;
        int32 ComponentsWithCollision = 0;
        int32 OverlapEventComponents = 0;
        int32 SimulatingPhysicsComponents = 0;
        int32 GravityDisabledPhysicsComponents = 0;
        int32 TickingComponents = 0;
        double TotalMassKg = 0.0;
    };

    TSharedPtr<FJsonObject> BuildTickInfo(const FTickFunction& TickFunction)
    {
        TSharedPtr<FJsonObject> TickInfo = MakeShared<FJsonObject>();
        TickInfo->SetBoolField(TEXT("can_ever_tick"), TickFunction.bCanEverTick);
        TickInfo->SetBoolField(TEXT("tick_enabled"), TickFunction.IsTickFunctionEnabled());
        TickInfo->SetBoolField(TEXT("start_with_tick_enabled"), TickFunction.bStartWithTickEnabled);
        TickInfo->SetBoolField(TEXT("allow_tick_on_dedicated_server"), TickFunction.bAllowTickOnDedicatedServer);
        TickInfo->SetBoolField(TEXT("tick_even_when_paused"), TickFunction.bTickEvenWhenPaused);
        TickInfo->SetBoolField(TEXT("high_priority"), TickFunction.bHighPriority);
        TickInfo->SetStringField(
            TEXT("tick_group"),
            GetEnumValueName(StaticEnum<ETickingGroup>(), static_cast<int64>(TickFunction.TickGroup))
        );
        return TickInfo;
    }

    TSharedPtr<FJsonObject> BuildComponentDiagnostics(
        UActorComponent* Component,
        const bool bIncludeAssets,
        const bool bIncludeRendering,
        const bool bIncludePhysics,
        const bool bIncludeTick,
        const bool bIncludeCollision,
        FActorFullInfoStats& Stats)
    {
        TSharedPtr<FJsonObject> ComponentObject = MakeShared<FJsonObject>();
        ComponentObject->SetStringField(TEXT("name"), Component->GetName());
        ComponentObject->SetStringField(TEXT("class"), Component->GetClass()->GetName());
        ComponentObject->SetStringField(TEXT("path"), Component->GetPathName());
        ComponentObject->SetBoolField(TEXT("is_registered"), Component->IsRegistered());
        ComponentObject->SetBoolField(TEXT("is_active"), Component->IsActive());
        ComponentObject->SetBoolField(TEXT("is_replicated"), Component->GetIsReplicated());
        ComponentObject->SetArrayField(TEXT("tags"), NameArrayToJson(Component->ComponentTags));

        if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
        {
            ComponentObject->SetArrayField(TEXT("relative_location"), VectorToJsonArray(SceneComponent->GetRelativeLocation()));
            ComponentObject->SetArrayField(TEXT("relative_rotation"), RotatorToJsonArray(SceneComponent->GetRelativeRotation()));
            ComponentObject->SetArrayField(TEXT("relative_scale"), VectorToJsonArray(SceneComponent->GetRelativeScale3D()));
            ComponentObject->SetArrayField(TEXT("world_location"), VectorToJsonArray(SceneComponent->GetComponentLocation()));
            ComponentObject->SetArrayField(TEXT("world_rotation"), RotatorToJsonArray(SceneComponent->GetComponentRotation()));
            ComponentObject->SetArrayField(TEXT("world_scale"), VectorToJsonArray(SceneComponent->GetComponentScale()));
            ComponentObject->SetStringField(
                TEXT("mobility"),
                GetEnumValueName(StaticEnum<EComponentMobility::Type>(), static_cast<int64>(SceneComponent->Mobility))
            );
        }

        if (bIncludeTick)
        {
            const TSharedPtr<FJsonObject> TickInfo = BuildTickInfo(Component->PrimaryComponentTick);
            ComponentObject->SetObjectField(TEXT("tick"), TickInfo);
            if (TickInfo->GetBoolField(TEXT("can_ever_tick")) && TickInfo->GetBoolField(TEXT("tick_enabled")))
            {
                ++Stats.TickingComponents;
            }
        }

        if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
        {
            ++Stats.PrimitiveComponentCount;

            if (bIncludeRendering)
            {
                const bool bVisible = PrimitiveComponent->IsVisible();
                const bool bCastShadow = PrimitiveComponent->CastShadow;
                ComponentObject->SetBoolField(TEXT("visible"), bVisible);
                ComponentObject->SetBoolField(TEXT("cast_shadow"), bCastShadow);

                if (bVisible)
                {
                    ++Stats.VisiblePrimitiveCount;
                }
                if (bCastShadow)
                {
                    ++Stats.ShadowCastingComponentCount;
                    if (const USceneComponent* SceneComponent = Cast<USceneComponent>(PrimitiveComponent))
                    {
                        if (SceneComponent->Mobility == EComponentMobility::Movable)
                        {
                            ++Stats.MovableShadowCasterCount;
                        }
                    }
                }
            }

            if (bIncludeCollision)
            {
                TSharedPtr<FJsonObject> CollisionObject = MakeShared<FJsonObject>();
                const ECollisionEnabled::Type CollisionEnabled = PrimitiveComponent->GetCollisionEnabled();
                const bool bComponentCollisionEnabled = CollisionEnabled != ECollisionEnabled::NoCollision;

                CollisionObject->SetStringField(
                    TEXT("collision_enabled"),
                    GetEnumValueName(StaticEnum<ECollisionEnabled::Type>(), static_cast<int64>(CollisionEnabled))
                );
                CollisionObject->SetStringField(TEXT("collision_profile"), PrimitiveComponent->GetCollisionProfileName().ToString());
                CollisionObject->SetBoolField(TEXT("generate_overlap_events"), PrimitiveComponent->GetGenerateOverlapEvents());
                CollisionObject->SetStringField(
                    TEXT("object_type"),
                    GetEnumValueName(StaticEnum<ECollisionChannel>(), static_cast<int64>(PrimitiveComponent->GetCollisionObjectType()))
                );

                TSharedPtr<FJsonObject> ResponsesObject = MakeShared<FJsonObject>();
                const UEnum* CollisionChannelEnum = StaticEnum<ECollisionChannel>();
                const UEnum* CollisionResponseEnum = StaticEnum<ECollisionResponse>();
                for (int32 ChannelIndex = 0; ChannelIndex < static_cast<int32>(ECollisionChannel::ECC_MAX); ++ChannelIndex)
                {
                    const ECollisionChannel Channel = static_cast<ECollisionChannel>(ChannelIndex);
                    const FString ChannelName = GetEnumValueName(CollisionChannelEnum, ChannelIndex);
                    const ECollisionResponse Response = PrimitiveComponent->GetCollisionResponseToChannel(Channel);
                    ResponsesObject->SetStringField(
                        ChannelName,
                        GetEnumValueName(CollisionResponseEnum, static_cast<int64>(Response))
                    );
                }
                CollisionObject->SetObjectField(TEXT("responses"), ResponsesObject);
                ComponentObject->SetObjectField(TEXT("collision"), CollisionObject);

                if (bComponentCollisionEnabled)
                {
                    ++Stats.ComponentsWithCollision;
                }
                if (PrimitiveComponent->GetGenerateOverlapEvents())
                {
                    ++Stats.OverlapEventComponents;
                }
            }

            if (bIncludePhysics)
            {
                const bool bIsSimulatingPhysics = PrimitiveComponent->IsSimulatingPhysics();
                const bool bGravityEnabled = PrimitiveComponent->IsGravityEnabled();
                const double Mass = PrimitiveComponent->GetMass();

                TSharedPtr<FJsonObject> PhysicsObject = MakeShared<FJsonObject>();
                PhysicsObject->SetBoolField(TEXT("simulate_physics"), bIsSimulatingPhysics);
                PhysicsObject->SetBoolField(TEXT("gravity_enabled"), bGravityEnabled);
                PhysicsObject->SetNumberField(TEXT("mass_kg"), Mass);
                PhysicsObject->SetNumberField(TEXT("linear_damping"), PrimitiveComponent->GetLinearDamping());
                PhysicsObject->SetNumberField(TEXT("angular_damping"), PrimitiveComponent->GetAngularDamping());
                ComponentObject->SetObjectField(TEXT("physics"), PhysicsObject);

                if (bIsSimulatingPhysics)
                {
                    ++Stats.SimulatingPhysicsComponents;
                    if (!bGravityEnabled)
                    {
                        ++Stats.GravityDisabledPhysicsComponents;
                    }
                }

                Stats.TotalMassKg += FMath::Max(0.0, Mass);
            }

            if (bIncludeAssets)
            {
                const int32 MaterialCount = PrimitiveComponent->GetNumMaterials();
                Stats.TotalMaterialSlots += MaterialCount;
                ComponentObject->SetNumberField(TEXT("material_slot_count"), MaterialCount);

                TArray<TSharedPtr<FJsonValue>> MaterialArray;
                for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
                {
                    TSharedPtr<FJsonObject> MaterialObject = MakeShared<FJsonObject>();
                    if (const UMaterialInterface* Material = PrimitiveComponent->GetMaterial(MaterialIndex))
                    {
                        MaterialObject->SetStringField(TEXT("name"), Material->GetName());
                        MaterialObject->SetStringField(TEXT("path"), Material->GetPathName());
                    }
                    else
                    {
                        MaterialObject->SetStringField(TEXT("name"), TEXT("None"));
                        MaterialObject->SetStringField(TEXT("path"), TEXT(""));
                    }
                    MaterialArray.Add(MakeShared<FJsonValueObject>(MaterialObject));
                }
                ComponentObject->SetArrayField(TEXT("materials"), MaterialArray);

                if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
                {
                    if (const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
                    {
                        ComponentObject->SetStringField(TEXT("static_mesh"), StaticMesh->GetPathName());
                        ComponentObject->SetNumberField(TEXT("static_mesh_lod_count"), StaticMesh->GetNumLODs());
                    }
                }
                else if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(PrimitiveComponent))
                {
                    if (const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
                    {
                        ComponentObject->SetStringField(TEXT("skeletal_mesh"), SkeletalMesh->GetPathName());
                        if (const UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset())
                        {
                            ComponentObject->SetStringField(TEXT("physics_asset"), PhysicsAsset->GetPathName());
                        }
                    }
                }
            }
        }

        return ComponentObject;
    }

    FString MakeAbsoluteProjectPath(const FString& InputPath)
    {
        FString ResolvedPath = InputPath;
        FPaths::NormalizeFilename(ResolvedPath);

        if (FPaths::IsRelative(ResolvedPath))
        {
            ResolvedPath = FPaths::Combine(FPaths::ProjectDir(), ResolvedPath);
        }

        ResolvedPath = FPaths::ConvertRelativePathToFull(ResolvedPath);
        FPaths::CollapseRelativeDirectories(ResolvedPath);
        return ResolvedPath;
    }

    FString SanitizeBaseFileName(const FString& NameCandidate)
    {
        FString BaseName = FPaths::GetBaseFilename(NameCandidate);
        if (BaseName.IsEmpty())
        {
            BaseName = NameCandidate;
        }

        BaseName.TrimStartAndEndInline();

        const TCHAR InvalidChars[] = { TEXT('\\'), TEXT('/'), TEXT(':'), TEXT('*'), TEXT('?'), TEXT('"'), TEXT('<'), TEXT('>'), TEXT('|') };
        for (TCHAR InvalidChar : InvalidChars)
        {
            BaseName.ReplaceCharInline(InvalidChar, TEXT('_'));
        }

        return BaseName.IsEmpty() ? TEXT("screenshot") : BaseName;
    }

    FString BuildDefaultScreenshotBaseName(const FString& Target)
    {
        return FString::Printf(
            TEXT("%s_%s"),
            *Target,
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    }

    bool TryGetResolutionFromParams(
        const TSharedPtr<FJsonObject>& Params,
        TOptional<FIntPoint>& OutResolution,
        FString& OutError)
    {
        if (!Params->HasField(TEXT("resolution")))
        {
            return true;
        }

        const TSharedPtr<FJsonObject>* ResolutionObjectPtr = nullptr;
        if (!Params->TryGetObjectField(TEXT("resolution"), ResolutionObjectPtr) || !ResolutionObjectPtr || !ResolutionObjectPtr->IsValid())
        {
            OutError = TEXT("Parameter 'resolution' must be an object with 'width' and 'height'");
            return false;
        }

        double WidthAsDouble = 0.0;
        double HeightAsDouble = 0.0;
        if (!(*ResolutionObjectPtr)->TryGetNumberField(TEXT("width"), WidthAsDouble) ||
            !(*ResolutionObjectPtr)->TryGetNumberField(TEXT("height"), HeightAsDouble))
        {
            OutError = TEXT("Parameter 'resolution' must include numeric 'width' and 'height'");
            return false;
        }

        const int32 Width = FMath::FloorToInt(WidthAsDouble);
        const int32 Height = FMath::FloorToInt(HeightAsDouble);
        if (Width <= 0 || Height <= 0 || !FMath::IsNearlyEqual(WidthAsDouble, static_cast<double>(Width)) ||
            !FMath::IsNearlyEqual(HeightAsDouble, static_cast<double>(Height)))
        {
            OutError = TEXT("Parameters 'resolution.width' and 'resolution.height' must be positive integers");
            return false;
        }

        OutResolution = FIntPoint(Width, Height);
        return true;
    }

    bool ResolveScreenshotOutputPath(
        const TSharedPtr<FJsonObject>& Params,
        const FString& Target,
        FString& OutScreenshotPath,
        FString& OutError)
    {
        FString FilePath;
        const bool bHasFilePath = Params->TryGetStringField(TEXT("file_path"), FilePath);
        if (bHasFilePath)
        {
            FilePath.TrimStartAndEndInline();
            if (FilePath.IsEmpty())
            {
                OutError = TEXT("Parameter 'file_path' cannot be empty when provided");
                return false;
            }
        }

        FString FileName;
        const bool bHasFileName = Params->TryGetStringField(TEXT("file_name"), FileName);
        if (bHasFileName)
        {
            FileName.TrimStartAndEndInline();
            if (FileName.IsEmpty())
            {
                OutError = TEXT("Parameter 'file_name' cannot be empty when provided");
                return false;
            }
        }

        FString DirectoryPath;
        FString BaseFileName;

        if (bHasFilePath)
        {
            const bool bEndsWithSeparator = FilePath.EndsWith(TEXT("/")) || FilePath.EndsWith(TEXT("\\"));
            const FString AbsoluteFilePath = MakeAbsoluteProjectPath(FilePath);
            const bool bExistingDirectory = IFileManager::Get().DirectoryExists(*AbsoluteFilePath);
            const bool bHasExtension = !FPaths::GetExtension(FilePath).IsEmpty();
            const bool bLikelyDirectoryPath = !bHasExtension && (FilePath.Contains(TEXT("/")) || FilePath.Contains(TEXT("\\")));
            const bool bTreatAsDirectory = bEndsWithSeparator || bExistingDirectory || bHasFileName || bLikelyDirectoryPath;

            if (bTreatAsDirectory)
            {
                DirectoryPath = AbsoluteFilePath;
                BaseFileName = bHasFileName ? SanitizeBaseFileName(FileName) : BuildDefaultScreenshotBaseName(Target);
            }
            else
            {
                DirectoryPath = FPaths::GetPath(AbsoluteFilePath);
                BaseFileName = SanitizeBaseFileName(AbsoluteFilePath);
            }
        }
        else
        {
            DirectoryPath = MakeAbsoluteProjectPath(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots")));
            BaseFileName = bHasFileName ? SanitizeBaseFileName(FileName) : BuildDefaultScreenshotBaseName(Target);
        }

        if (DirectoryPath.IsEmpty())
        {
            OutError = TEXT("Could not resolve screenshot output directory");
            return false;
        }

        IFileManager& FileManager = IFileManager::Get();
        if (!FileManager.DirectoryExists(*DirectoryPath) && !FileManager.MakeDirectory(*DirectoryPath, true))
        {
            OutError = FString::Printf(TEXT("Failed to create screenshot directory: %s"), *DirectoryPath);
            return false;
        }

        if (!FileManager.DirectoryExists(*DirectoryPath))
        {
            OutError = FString::Printf(TEXT("Screenshot directory is not accessible: %s"), *DirectoryPath);
            return false;
        }

        FString CandidatePath = FPaths::Combine(DirectoryPath, BaseFileName + TEXT(".png"));
        int32 Suffix = 1;
        while (FileManager.FileExists(*CandidatePath))
        {
            CandidatePath = FPaths::Combine(DirectoryPath, FString::Printf(TEXT("%s_%03d.png"), *BaseFileName, Suffix));
            ++Suffix;
        }

        OutScreenshotPath = CandidatePath;
        return true;
    }

    bool CaptureEditorWindowPixels(TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError)
    {
        if (!FSlateApplication::IsInitialized())
        {
            OutError = TEXT("Slate application is not initialized");
            return false;
        }

        TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
        if (!ActiveWindow.IsValid())
        {
            const TArray<TSharedRef<SWindow>> InteractiveWindows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
            for (const TSharedRef<SWindow>& CandidateWindow : InteractiveWindows)
            {
                if (CandidateWindow->IsVisible())
                {
                    ActiveWindow = CandidateWindow;
                    break;
                }
            }
        }

        if (!ActiveWindow.IsValid())
        {
            const TArray<TSharedRef<SWindow>> TopLevelWindows = FSlateApplication::Get().GetTopLevelWindows();
            for (const TSharedRef<SWindow>& CandidateWindow : TopLevelWindows)
            {
                if (CandidateWindow->IsVisible())
                {
                    ActiveWindow = CandidateWindow;
                    break;
                }
            }
        }

        if (!ActiveWindow.IsValid())
        {
            OutError = TEXT("No editor window available for screenshot");
            return false;
        }

        FIntVector OutImageSize;
        if (!FSlateApplication::Get().TakeScreenshot(ActiveWindow.ToSharedRef(), OutPixels, OutImageSize))
        {
            OutError = TEXT("Failed to capture the active editor window");
            return false;
        }

        OutWidth = OutImageSize.X;
        OutHeight = OutImageSize.Y;
        return OutWidth > 0 && OutHeight > 0;
    }

    bool CaptureActiveViewportPixels(TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError)
    {
        if (!GEditor || !GEditor->GetActiveViewport())
        {
            OutError = TEXT("No active viewport available for screenshot");
            return false;
        }

        FViewport* Viewport = GEditor->GetActiveViewport();
        const FIntPoint ViewportSize = Viewport->GetSizeXY();
        if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
        {
            OutError = TEXT("Active viewport has invalid size");
            return false;
        }

        const FIntRect ViewportRect(0, 0, ViewportSize.X, ViewportSize.Y);
        if (!Viewport->ReadPixels(OutPixels, FReadSurfaceDataFlags(), ViewportRect))
        {
            OutError = TEXT("Failed to read pixels from active viewport");
            return false;
        }

        OutWidth = ViewportSize.X;
        OutHeight = ViewportSize.Y;
        return true;
    }

    bool ApplyRequestedResolution(
        TArray<FColor>& InOutPixels,
        int32& InOutWidth,
        int32& InOutHeight,
        const TOptional<FIntPoint>& RequestedResolution,
        FString& OutError)
    {
        if (!RequestedResolution.IsSet())
        {
            return true;
        }

        const int32 TargetWidth = RequestedResolution->X;
        const int32 TargetHeight = RequestedResolution->Y;
        if (InOutWidth == TargetWidth && InOutHeight == TargetHeight)
        {
            return true;
        }

        TArray<FColor> ResizedPixels;
        FImageUtils::ImageResize(
            InOutWidth,
            InOutHeight,
            InOutPixels,
            TargetWidth,
            TargetHeight,
            ResizedPixels,
            true);

        if (ResizedPixels.Num() == 0)
        {
            OutError = TEXT("Failed to resize screenshot to requested resolution");
            return false;
        }

        InOutPixels = MoveTemp(ResizedPixels);
        InOutWidth = TargetWidth;
        InOutHeight = TargetHeight;
        return true;
    }

    void EnsureOpaqueAlpha(TArray<FColor>& InOutPixels)
    {
        for (FColor& Pixel : InOutPixels)
        {
            Pixel.A = 255;
        }
    }
}

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("get_actor_full_info"))
    {
        return HandleGetActorFullInfo(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    else if (CommandType == TEXT("set_actor_component_property"))
    {
        return HandleSetActorComponentProperty(Params);
    }
    else if (CommandType == TEXT("fix_invalid_nanite_materials"))
    {
        return HandleFixInvalidNaniteMaterials(Params);
    }
    else if (CommandType == TEXT("open_level"))
    {
        return HandleOpenLevel(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("capture_editor_screenshot"))
    {
        return HandleCaptureEditorScreenshot(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorFullInfo(const TSharedPtr<FJsonObject>& Params)
{
    if (!Params.IsValid())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing command parameters"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    bool bIncludeComponents = true;
    bool bIncludeAssets = true;
    bool bIncludeRendering = true;
    bool bIncludePhysics = true;
    bool bIncludeTick = true;
    bool bIncludeCollision = true;

    FString ValidationError;
    const auto ReadOptionalBool = [&Params, &ValidationError](const TCHAR* FieldName, bool& OutValue) -> bool
    {
        if (!Params->HasField(FieldName))
        {
            return true;
        }

        if (!Params->HasTypedField<EJson::Boolean>(FieldName))
        {
            ValidationError = FString::Printf(TEXT("Parameter '%s' must be a boolean"), FieldName);
            return false;
        }

        OutValue = Params->GetBoolField(FieldName);
        return true;
    };

    if (!ReadOptionalBool(TEXT("include_components"), bIncludeComponents) ||
        !ReadOptionalBool(TEXT("include_assets"), bIncludeAssets) ||
        !ReadOptionalBool(TEXT("include_rendering"), bIncludeRendering) ||
        !ReadOptionalBool(TEXT("include_physics"), bIncludePhysics) ||
        !ReadOptionalBool(TEXT("include_tick"), bIncludeTick) ||
        !ReadOptionalBool(TEXT("include_collision"), bIncludeCollision))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ValidationError);
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : GWorld;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    TArray<AActor*> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            MatchingActors.Add(Actor);
        }
    }

    if (MatchingActors.Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    if (MatchingActors.Num() > 1)
    {
        TArray<FString> CollisionPaths;
        for (const AActor* CollisionActor : MatchingActors)
        {
            CollisionPaths.Add(CollisionActor->GetPathName());
        }
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(
                TEXT("Multiple actors found for name '%s': %s"),
                *ActorName,
                *FString::Join(CollisionPaths, TEXT(", "))
            )
        );
    }

    AActor* TargetActor = MatchingActors[0];
    FActorFullInfoStats Stats;
    TArray<FString> OptimizationHints;
    TArray<FString> Warnings;

    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), TargetActor->GetName());
    ActorObject->SetStringField(TEXT("label"), TargetActor->GetActorNameOrLabel());
    ActorObject->SetStringField(TEXT("class"), TargetActor->GetClass()->GetName());
    ActorObject->SetStringField(TEXT("path"), TargetActor->GetPathName());
    ActorObject->SetStringField(TEXT("world"), World->GetName());
    ActorObject->SetStringField(TEXT("level"), TargetActor->GetLevel() ? TargetActor->GetLevel()->GetName() : TEXT(""));
    ActorObject->SetStringField(TEXT("folder_path"), TargetActor->GetFolderPath().ToString());
    ActorObject->SetArrayField(TEXT("tags"), NameArrayToJson(TargetActor->Tags));

    ActorObject->SetArrayField(TEXT("location"), VectorToJsonArray(TargetActor->GetActorLocation()));
    ActorObject->SetArrayField(TEXT("rotation"), RotatorToJsonArray(TargetActor->GetActorRotation()));
    ActorObject->SetArrayField(TEXT("scale"), VectorToJsonArray(TargetActor->GetActorScale3D()));

    FVector BoundsOrigin(0.0f, 0.0f, 0.0f);
    FVector BoundsExtent(0.0f, 0.0f, 0.0f);
    TargetActor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
    TSharedPtr<FJsonObject> BoundsObject = MakeShared<FJsonObject>();
    BoundsObject->SetArrayField(TEXT("origin"), VectorToJsonArray(BoundsOrigin));
    BoundsObject->SetArrayField(TEXT("extent"), VectorToJsonArray(BoundsExtent));
    BoundsObject->SetNumberField(TEXT("sphere_radius"), BoundsExtent.Size());
    ActorObject->SetObjectField(TEXT("bounds"), BoundsObject);

    ActorObject->SetBoolField(TEXT("hidden"), TargetActor->IsHidden());
    ActorObject->SetBoolField(TEXT("actor_enable_collision"), TargetActor->GetActorEnableCollision());
    ActorObject->SetBoolField(TEXT("is_editor_only_actor"), TargetActor->IsEditorOnly());
    ActorObject->SetBoolField(TEXT("is_replicated"), TargetActor->GetIsReplicated());
    ActorObject->SetNumberField(TEXT("component_count"), TargetActor->GetComponents().Num());

    if (const USceneComponent* RootScene = Cast<USceneComponent>(TargetActor->GetRootComponent()))
    {
        ActorObject->SetStringField(TEXT("root_component"), RootScene->GetName());
        ActorObject->SetStringField(
            TEXT("root_mobility"),
            GetEnumValueName(StaticEnum<EComponentMobility::Type>(), static_cast<int64>(RootScene->Mobility))
        );
    }

    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    if (bIncludeComponents)
    {
        TArray<UActorComponent*> Components;
        TargetActor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                Warnings.Add(TEXT("Encountered null component while collecting diagnostics"));
                continue;
            }

            ComponentArray.Add(MakeShared<FJsonValueObject>(
                BuildComponentDiagnostics(
                    Component,
                    bIncludeAssets,
                    bIncludeRendering,
                    bIncludePhysics,
                    bIncludeTick,
                    bIncludeCollision,
                    Stats)));
        }
    }

    if (bIncludeTick)
    {
        const TSharedPtr<FJsonObject> ActorTickInfo = BuildTickInfo(TargetActor->PrimaryActorTick);
        if (TargetActor->PrimaryActorTick.bCanEverTick && TargetActor->PrimaryActorTick.IsTickFunctionEnabled() && TargetActor->IsHidden())
        {
            OptimizationHints.Add(TEXT("Actor is hidden but actor tick is enabled"));
        }

        if (Stats.TickingComponents > 15)
        {
            OptimizationHints.Add(FString::Printf(TEXT("High ticking component count: %d"), Stats.TickingComponents));
        }

        ActorObject->SetObjectField(TEXT("tick"), ActorTickInfo);
    }

    if (bIncludeRendering && Stats.MovableShadowCasterCount > 4)
    {
        OptimizationHints.Add(FString::Printf(TEXT("Multiple movable shadow casters detected: %d"), Stats.MovableShadowCasterCount));
    }

    if (bIncludeAssets && Stats.TotalMaterialSlots > 24)
    {
        OptimizationHints.Add(FString::Printf(TEXT("High material slot count across components: %d"), Stats.TotalMaterialSlots));
    }

    if (bIncludePhysics && Stats.SimulatingPhysicsComponents > 8)
    {
        OptimizationHints.Add(FString::Printf(TEXT("High number of physics-simulating components: %d"), Stats.SimulatingPhysicsComponents));
    }

    if (bIncludeCollision && TargetActor->IsHidden() && Stats.ComponentsWithCollision > 10)
    {
        OptimizationHints.Add(TEXT("Hidden actor keeps many collision-enabled components"));
    }

    TSharedPtr<FJsonObject> RenderingObject = MakeShared<FJsonObject>();
    RenderingObject->SetBoolField(TEXT("enabled"), bIncludeRendering);
    RenderingObject->SetNumberField(TEXT("primitive_component_count"), Stats.PrimitiveComponentCount);
    RenderingObject->SetNumberField(TEXT("visible_primitive_component_count"), Stats.VisiblePrimitiveCount);
    RenderingObject->SetNumberField(TEXT("shadow_casting_component_count"), Stats.ShadowCastingComponentCount);
    RenderingObject->SetNumberField(TEXT("movable_shadow_caster_count"), Stats.MovableShadowCasterCount);
    RenderingObject->SetNumberField(TEXT("material_slot_count"), Stats.TotalMaterialSlots);

    TSharedPtr<FJsonObject> PhysicsObject = MakeShared<FJsonObject>();
    PhysicsObject->SetBoolField(TEXT("enabled"), bIncludePhysics);
    PhysicsObject->SetNumberField(TEXT("simulating_component_count"), Stats.SimulatingPhysicsComponents);
    PhysicsObject->SetNumberField(TEXT("gravity_disabled_component_count"), Stats.GravityDisabledPhysicsComponents);
    PhysicsObject->SetNumberField(TEXT("total_mass_kg"), Stats.TotalMassKg);

    TSharedPtr<FJsonObject> TickObject = MakeShared<FJsonObject>();
    TickObject->SetBoolField(TEXT("enabled"), bIncludeTick);
    TickObject->SetNumberField(TEXT("ticking_component_count"), Stats.TickingComponents);
    if (bIncludeTick)
    {
        TickObject->SetObjectField(TEXT("actor_tick"), BuildTickInfo(TargetActor->PrimaryActorTick));
    }

    TSharedPtr<FJsonObject> CollisionObject = MakeShared<FJsonObject>();
    CollisionObject->SetBoolField(TEXT("enabled"), bIncludeCollision);
    CollisionObject->SetBoolField(TEXT("actor_collision_enabled"), TargetActor->GetActorEnableCollision());
    CollisionObject->SetNumberField(TEXT("collision_enabled_component_count"), Stats.ComponentsWithCollision);
    CollisionObject->SetNumberField(TEXT("overlap_event_component_count"), Stats.OverlapEventComponents);

    TArray<TSharedPtr<FJsonValue>> OptimizationHintArray;
    for (const FString& Hint : OptimizationHints)
    {
        OptimizationHintArray.Add(MakeShared<FJsonValueString>(Hint));
    }

    TArray<TSharedPtr<FJsonValue>> WarningArray;
    for (const FString& Warning : Warnings)
    {
        WarningArray.Add(MakeShared<FJsonValueString>(Warning));
    }

    TSharedPtr<FJsonObject> ResultObject = MakeShared<FJsonObject>();
    ResultObject->SetStringField(TEXT("command"), TEXT("get_actor_full_info"));
    ResultObject->SetObjectField(TEXT("actor"), ActorObject);
    ResultObject->SetArrayField(TEXT("components"), ComponentArray);
    ResultObject->SetObjectField(TEXT("rendering"), RenderingObject);
    ResultObject->SetObjectField(TEXT("physics"), PhysicsObject);
    ResultObject->SetObjectField(TEXT("tick"), TickObject);
    ResultObject->SetObjectField(TEXT("collision"), CollisionObject);
    ResultObject->SetArrayField(TEXT("optimization_hints"), OptimizationHintArray);
    ResultObject->SetArrayField(TEXT("warnings"), WarningArray);
    return ResultObject;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString ActorLookupError;
    AActor* TargetActor = FindActorByQuery(ActorName, ActorLookupError);
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ActorLookupError);
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

    auto BuildSuccessResult = [&](UObject* TargetObject, const FString& TargetType, const FString& ResolvedProperty) -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("actor"), TargetActor->GetName());
        ResultObj->SetStringField(TEXT("target_type"), TargetType);
        ResultObj->SetStringField(TEXT("target_name"), TargetObject ? TargetObject->GetName() : TEXT("<null>"));
        ResultObj->SetStringField(TEXT("property"), ResolvedProperty);
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    };

    auto TrySetProperty = [&](UObject* TargetObject, const FString& RequestedProperty, FString& OutFailureReason) -> bool
    {
        if (!TargetObject)
        {
            OutFailureReason = TEXT("Target object is null");
            return false;
        }

        if (!FUnrealMCPCommonUtils::SetObjectProperty(TargetObject, RequestedProperty, PropertyValue, OutFailureReason))
        {
            return false;
        }

        MarkObjectEdited(TargetObject);
        return true;
    };

    FString ExplicitComponentQuery;
    Params->TryGetStringField(TEXT("component_name"), ExplicitComponentQuery);
    ExplicitComponentQuery.TrimStartAndEndInline();

    if (!ExplicitComponentQuery.IsEmpty())
    {
        FString ComponentLookupError;
        UActorComponent* TargetComponent = FUnrealMCPCommonUtils::FindActorComponent(TargetActor, ExplicitComponentQuery, ComponentLookupError);
        if (!TargetComponent)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(ComponentLookupError);
        }

        FString ComponentSetError;
        if (!TrySetProperty(TargetComponent, PropertyName, ComponentSetError))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(ComponentSetError);
        }

        return BuildSuccessResult(TargetComponent, TEXT("component"), PropertyName);
    }

    const int32 DotIndex = PropertyName.Find(TEXT("."));
    if (DotIndex > 0)
    {
        const FString ComponentQuery = PropertyName.Left(DotIndex);
        const FString NestedProperty = PropertyName.Mid(DotIndex + 1);
        if (!ComponentQuery.IsEmpty() && !NestedProperty.IsEmpty())
        {
            FString ComponentLookupError;
            if (UActorComponent* ComponentFromPath = FUnrealMCPCommonUtils::FindActorComponent(TargetActor, ComponentQuery, ComponentLookupError))
            {
                FString SetError;
                if (!TrySetProperty(ComponentFromPath, NestedProperty, SetError))
                {
                    return FUnrealMCPCommonUtils::CreateErrorResponse(SetError);
                }

                return BuildSuccessResult(ComponentFromPath, TEXT("component"), NestedProperty);
            }
        }
    }

    FString ActorSetError;
    if (TrySetProperty(TargetActor, PropertyName, ActorSetError))
    {
        return BuildSuccessResult(TargetActor, TEXT("actor"), PropertyName);
    }

    if (!ActorSetError.Contains(TEXT("Property not found"), ESearchCase::IgnoreCase))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ActorSetError);
    }

    if (USceneComponent* RootComponent = TargetActor->GetRootComponent())
    {
        if (FUnrealMCPCommonUtils::HasObjectProperty(RootComponent, PropertyName))
        {
            FString RootSetError;
            if (!TrySetProperty(RootComponent, PropertyName, RootSetError))
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(RootSetError);
            }

            return BuildSuccessResult(RootComponent, TEXT("root_component"), PropertyName);
        }
    }

    TInlineComponentArray<UActorComponent*> Components(TargetActor);
    TArray<UActorComponent*> MatchingComponents;
    for (UActorComponent* Component : Components)
    {
        if (Component && FUnrealMCPCommonUtils::HasObjectProperty(Component, PropertyName))
        {
            MatchingComponents.Add(Component);
        }
    }

    if (MatchingComponents.Num() == 1)
    {
        FString ComponentSetError;
        if (!TrySetProperty(MatchingComponents[0], PropertyName, ComponentSetError))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(ComponentSetError);
        }

        return BuildSuccessResult(MatchingComponents[0], TEXT("component_auto"), PropertyName);
    }

    if (MatchingComponents.Num() > 1)
    {
        FString Candidates;
        for (UActorComponent* Component : MatchingComponents)
        {
            if (!Candidates.IsEmpty())
            {
                Candidates += TEXT(", ");
            }
            Candidates += FString::Printf(TEXT("%s(%s)"), *Component->GetName(), *Component->GetClass()->GetName());
        }

        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(
                TEXT("Property '%s' exists on multiple components of actor '%s'. Provide 'component_name'. Candidates: %s"),
                *PropertyName,
                *TargetActor->GetName(),
                *Candidates));
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(ActorSetError);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    FString ActorLookupError;
    AActor* TargetActor = FindActorByQuery(ActorName, ActorLookupError);
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ActorLookupError);
    }

    FString ComponentLookupError;
    UActorComponent* TargetComponent = FUnrealMCPCommonUtils::FindActorComponent(TargetActor, ComponentName, ComponentLookupError);
    if (!TargetComponent)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ComponentLookupError);
    }

    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));
    FString ErrorMessage;
    if (!FUnrealMCPCommonUtils::SetObjectProperty(TargetComponent, PropertyName, PropertyValue, ErrorMessage))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }

    MarkObjectEdited(TargetComponent);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetStringField(TEXT("component"), TargetComponent->GetName());
    ResultObj->SetStringField(TEXT("property"), PropertyName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFixInvalidNaniteMaterials(const TSharedPtr<FJsonObject>& Params)
{
    bool bDryRun = false;
    Params->TryGetBoolField(TEXT("dry_run"), bDryRun);

    bool bSaveDirtyPackages = false;
    Params->TryGetBoolField(TEXT("save_dirty_packages"), bSaveDirtyPackages);

    bool bOnlyIfNaniteEnabled = true;
    Params->TryGetBoolField(TEXT("only_if_nanite_enabled"), bOnlyIfNaniteEnabled);

    int32 MaxResultEntries = 200;
    if (Params->HasTypedField<EJson::Number>(TEXT("max_result_entries")))
    {
        MaxResultEntries = FMath::Max(0, static_cast<int32>(Params->GetNumberField(TEXT("max_result_entries"))));
    }

    FString ActorNameFilter;
    Params->TryGetStringField(TEXT("actor_name_filter"), ActorNameFilter);
    ActorNameFilter.TrimStartAndEndInline();

    auto ParseStringFilters = [&](const TCHAR* FieldName, TArray<FString>& OutFilters)
    {
        OutFilters.Reset();

        FString SingleValue;
        if (Params->TryGetStringField(FieldName, SingleValue))
        {
            SingleValue.TrimStartAndEndInline();
            if (!SingleValue.IsEmpty())
            {
                OutFilters.Add(SingleValue);
            }
            return;
        }

        const TArray<TSharedPtr<FJsonValue>>* ArrayValue = nullptr;
        if (!Params->TryGetArrayField(FieldName, ArrayValue) || !ArrayValue)
        {
            return;
        }

        for (const TSharedPtr<FJsonValue>& Entry : *ArrayValue)
        {
            if (!Entry.IsValid() || Entry->Type != EJson::String)
            {
                continue;
            }

            FString Filter = Entry->AsString();
            Filter.TrimStartAndEndInline();
            if (!Filter.IsEmpty())
            {
                OutFilters.Add(Filter);
            }
        }
    };

    TArray<FString> MeshNameFilters;
    TArray<FString> MaterialNameFilters;
    ParseStringFilters(TEXT("mesh_name_filters"), MeshNameFilters);
    ParseStringFilters(TEXT("material_name_filters"), MaterialNameFilters);

    auto MatchesAnyFilter = [](const FString& Candidate, const TArray<FString>& Filters) -> bool
    {
        if (Filters.Num() == 0)
        {
            return true;
        }

        for (const FString& Filter : Filters)
        {
            if (Candidate.Contains(Filter, ESearchCase::IgnoreCase))
            {
                return true;
            }
        }

        return false;
    };

    auto MatchesActorFilter = [&](AActor* Actor) -> bool
    {
        if (ActorNameFilter.IsEmpty() || !Actor)
        {
            return true;
        }

        const FString ActorName = Actor->GetName();
        const FString ActorPath = Actor->GetPathName();
#if WITH_EDITOR
        const FString ActorLabel = Actor->GetActorLabel(false);
#else
        const FString ActorLabel;
#endif

        return ActorName.Contains(ActorNameFilter, ESearchCase::IgnoreCase) ||
            ActorPath.Contains(ActorNameFilter, ESearchCase::IgnoreCase) ||
            ActorLabel.Contains(ActorNameFilter, ESearchCase::IgnoreCase);
    };

    int32 ScannedActors = 0;
    int32 ScannedComponents = 0;
    int32 MatchedComponents = 0;
    int32 UpdatedComponents = 0;
    int32 AlreadyDisallowedComponents = 0;

    TArray<TSharedPtr<FJsonValue>> MatchEntries;

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (!Actor || !MatchesActorFilter(Actor))
        {
            continue;
        }

        ScannedActors++;

        TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents(Actor);
        for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
        {
            ScannedComponents++;

            if (!StaticMeshComponent)
            {
                continue;
            }

            UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
            if (!StaticMesh)
            {
                continue;
            }

            const FString MeshName = StaticMesh->GetName();
            const FString MeshPath = StaticMesh->GetPathName();
            if (MeshNameFilters.Num() > 0 &&
                !MatchesAnyFilter(MeshName, MeshNameFilters) &&
                !MatchesAnyFilter(MeshPath, MeshNameFilters))
            {
                continue;
            }

            if (bOnlyIfNaniteEnabled && !StaticMesh->IsNaniteEnabled())
            {
                continue;
            }

            bool bHasInvalidMaterial = false;
            TArray<TSharedPtr<FJsonValue>> InvalidMaterialEntries;
            const int32 MaterialSlotCount = StaticMeshComponent->GetNumMaterials();

            for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlotCount; ++MaterialIndex)
            {
                UMaterialInterface* MaterialInterface = StaticMeshComponent->GetMaterial(MaterialIndex);
                if (!MaterialInterface)
                {
                    continue;
                }

                const FString MaterialName = MaterialInterface->GetName();
                const FString MaterialPath = MaterialInterface->GetPathName();
                if (MaterialNameFilters.Num() > 0 &&
                    !MatchesAnyFilter(MaterialName, MaterialNameFilters) &&
                    !MatchesAnyFilter(MaterialPath, MaterialNameFilters))
                {
                    continue;
                }

                const EBlendMode BlendMode = MaterialInterface->GetBlendMode();
                const bool bSupportsNanite = BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked;
                if (bSupportsNanite)
                {
                    continue;
                }

                bHasInvalidMaterial = true;

                TSharedPtr<FJsonObject> MaterialObj = MakeShared<FJsonObject>();
                MaterialObj->SetNumberField(TEXT("slot_index"), MaterialIndex);
                MaterialObj->SetStringField(TEXT("name"), MaterialName);
                MaterialObj->SetStringField(TEXT("path"), MaterialPath);
                MaterialObj->SetStringField(TEXT("blend_mode"), GetEnumValueName(StaticEnum<EBlendMode>(), static_cast<int64>(BlendMode)));
                InvalidMaterialEntries.Add(MakeShared<FJsonValueObject>(MaterialObj));
            }

            if (!bHasInvalidMaterial)
            {
                continue;
            }

            MatchedComponents++;

            FString Action;
            if (StaticMeshComponent->bDisallowNanite)
            {
                AlreadyDisallowedComponents++;
                Action = TEXT("already_disallowed");
            }
            else if (bDryRun)
            {
                Action = TEXT("would_update");
            }
            else
            {
                StaticMeshComponent->Modify();
                StaticMeshComponent->bDisallowNanite = true;
                StaticMeshComponent->MarkRenderStateDirty();
                StaticMeshComponent->MarkPackageDirty();
                Actor->MarkPackageDirty();
                UpdatedComponents++;
                Action = TEXT("updated");
            }

            if (MatchEntries.Num() < MaxResultEntries)
            {
                TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
                EntryObj->SetStringField(TEXT("actor"), Actor->GetName());
                EntryObj->SetStringField(TEXT("component"), StaticMeshComponent->GetName());
                EntryObj->SetStringField(TEXT("mesh_name"), MeshName);
                EntryObj->SetStringField(TEXT("mesh_path"), MeshPath);
                EntryObj->SetStringField(TEXT("action"), Action);
                EntryObj->SetArrayField(TEXT("invalid_materials"), InvalidMaterialEntries);
                MatchEntries.Add(MakeShared<FJsonValueObject>(EntryObj));
            }
        }
    }

    bool bSaveAttempted = false;
    bool bSaveSucceeded = false;
    if (!bDryRun && bSaveDirtyPackages)
    {
        bSaveAttempted = true;
        bSaveSucceeded = FEditorFileUtils::SaveDirtyPackages(
            false, // bPromptUserToSave
            true,  // bSaveMapPackages
            false, // bSaveContentPackages
            true,  // bFastSave
            false, // bNotifyNoPackagesSaved
            false  // bCanBeDeclined
        );
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetBoolField(TEXT("dry_run"), bDryRun);
    ResultObj->SetBoolField(TEXT("save_attempted"), bSaveAttempted);
    ResultObj->SetBoolField(TEXT("save_succeeded"), bSaveSucceeded);
    ResultObj->SetNumberField(TEXT("scanned_actors"), ScannedActors);
    ResultObj->SetNumberField(TEXT("scanned_components"), ScannedComponents);
    ResultObj->SetNumberField(TEXT("matched_components"), MatchedComponents);
    ResultObj->SetNumberField(TEXT("updated_components"), UpdatedComponents);
    ResultObj->SetNumberField(TEXT("already_disallowed_components"), AlreadyDisallowedComponents);
    ResultObj->SetBoolField(TEXT("result_truncated"), MatchEntries.Num() < MatchedComponents);
    ResultObj->SetArrayField(TEXT("matches"), MatchEntries);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleOpenLevel(const TSharedPtr<FJsonObject>& Params)
{
    FString MapPath;
    if (!Params->TryGetStringField(TEXT("map_path"), MapPath))
    {
        Params->TryGetStringField(TEXT("level_name"), MapPath);
    }

    if (MapPath.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'map_path' parameter"));
    }

    FString LongPackageName = MapPath;

    // Accept filesystem .umap paths as well as package paths.
    if (MapPath.EndsWith(TEXT(".umap")))
    {
        if (!FPackageName::TryConvertFilenameToLongPackageName(MapPath, LongPackageName))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Could not convert map filename to package path: %s"), *MapPath)
            );
        }
    }
    // Resolve a short name (e.g. "Hospital1") via the asset registry.
    else if (!MapPath.StartsWith(TEXT("/")))
    {
        FAssetRegistryModule& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

        FARFilter Filter;
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(TEXT("/Game"));
        Filter.bRecursivePaths = true;

        TArray<FAssetData> WorldAssets;
        AssetRegistryModule.Get().GetAssets(Filter, WorldAssets);

        TArray<FString> Matches;
        for (const FAssetData& AssetData : WorldAssets)
        {
            if (AssetData.AssetName.ToString().Equals(MapPath, ESearchCase::IgnoreCase))
            {
                Matches.Add(AssetData.PackageName.ToString());
            }
        }

        if (Matches.Num() == 0)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(
                    TEXT("Map '%s' not found. Use a full package path such as '/Game/EmergencyRoom/Maps/Hospital1'."),
                    *MapPath
                )
            );
        }

        if (Matches.Num() > 1)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Map name '%s' is ambiguous. Provide full package path."), *MapPath)
            );
        }

        LongPackageName = Matches[0];
    }

    FString MapFilename;
    if (!FPackageName::DoesPackageExist(LongPackageName, &MapFilename))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Map package does not exist: %s"), *LongPackageName)
        );
    }

    const bool bLoaded = FEditorFileUtils::LoadMap(MapFilename, false, true);
    if (!bLoaded)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to open map: %s"), *LongPackageName)
        );
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("map_path"), LongPackageName);
    ResultObj->SetStringField(TEXT("map_filename"), MapFilename);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString Root      = TEXT("/Game/Blueprints/");
    FString AssetPath = Root + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found – it must reside under /Game/Blueprints"), *BlueprintName));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCaptureEditorScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FString Target;
    if (!Params->TryGetStringField(TEXT("target"), Target))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target' parameter"));
    }

    Target.TrimStartAndEndInline();
    Target = Target.ToLower();
    if (Target != TEXT("editor") && Target != TEXT("viewport"))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter 'target' must be 'editor' or 'viewport'"));
    }

    bool bShowUI = true;
    if (Params->HasField(TEXT("show_ui")) && !Params->TryGetBoolField(TEXT("show_ui"), bShowUI))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter 'show_ui' must be a boolean"));
    }

    TOptional<FIntPoint> RequestedResolution;
    FString ErrorMessage;
    if (!TryGetResolutionFromParams(Params, RequestedResolution, ErrorMessage))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }

    FString ScreenshotPath;
    if (!ResolveScreenshotOutputPath(Params, Target, ScreenshotPath, ErrorMessage))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }

    int32 Width = 0;
    int32 Height = 0;
    TArray<FColor> Pixels;

    if (Target == TEXT("editor"))
    {
        if (!CaptureEditorWindowPixels(Pixels, Width, Height, ErrorMessage))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
        }
    }
    else
    {
        if (!CaptureActiveViewportPixels(Pixels, Width, Height, ErrorMessage))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
        }
    }

    if (!ApplyRequestedResolution(Pixels, Width, Height, RequestedResolution, ErrorMessage))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }

    // Some viewport capture paths can return fully transparent alpha even with valid RGB data.
    // Force opaque alpha so saved PNGs render correctly across image viewers.
    EnsureOpaqueAlpha(Pixels);

    TArray64<uint8> CompressedBitmap;
    const TArrayView64<const FColor> PixelView(Pixels.GetData(), Pixels.Num());
    FImageUtils::PNGCompressImageArray(Width, Height, PixelView, CompressedBitmap);
    if (CompressedBitmap.Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to compress screenshot data"));
    }

    if (!FFileHelper::SaveArrayToFile(CompressedBitmap, *ScreenshotPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to save screenshot to '%s'"), *ScreenshotPath));
    }

    FString Message = FString::Printf(TEXT("Screenshot captured successfully to '%s'"), *ScreenshotPath);
    if (Target == TEXT("editor") && !bShowUI)
    {
        Message += TEXT(". 'show_ui' is ignored for editor target and UI is always included");
    }
    else if (Target == TEXT("viewport") && bShowUI)
    {
        Message += TEXT(". Viewport capture includes UI overlays only when rendered into the viewport");
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("message"), Message);
    ResultObj->SetStringField(TEXT("screenshot_path"), ScreenshotPath);
    ResultObj->SetStringField(TEXT("target"), Target);
    ResultObj->SetNumberField(TEXT("width"), Width);
    ResultObj->SetNumberField(TEXT("height"), Height);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FString LegacyFilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), LegacyFilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }

    TSharedPtr<FJsonObject> NormalizedParams = MakeShared<FJsonObject>();
    NormalizedParams->SetStringField(TEXT("target"), TEXT("viewport"));
    NormalizedParams->SetStringField(TEXT("file_path"), LegacyFilePath);

    bool bShowUI = true;
    if (Params->TryGetBoolField(TEXT("show_ui"), bShowUI))
    {
        NormalizedParams->SetBoolField(TEXT("show_ui"), bShowUI);
    }

    const TSharedPtr<FJsonObject>* ResolutionObject = nullptr;
    if (Params->TryGetObjectField(TEXT("resolution"), ResolutionObject) && ResolutionObject && ResolutionObject->IsValid())
    {
        NormalizedParams->SetObjectField(TEXT("resolution"), *ResolutionObject);
    }

    return HandleCaptureEditorScreenshot(NormalizedParams);
}
