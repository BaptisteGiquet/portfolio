#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyCharacterSelectionWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Framework/EMRLobbyGameState.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GAS/EMRTags.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Lobby/EMRLobbyCharacterPreviewStage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Subsystems/EMRLobbyCharacterSelectionSubsystem.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyScreenWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

const FName UEMRFrontendLobbyCharacterSelectionWidget::PreviewTextureParamName(TEXT("PreviewTexture"));

void UEMRFrontendLobbyCharacterSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
        true,
        FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));

    BindInput();
}

void UEMRFrontendLobbyCharacterSelectionWidget::BindInput()
{
    if (CommonButton_Left)
    {
        CommonButton_Left->OnClicked().RemoveAll(this);
        CommonButton_Left->OnClicked().AddUObject(this, &ThisClass::HandleLeftClicked);
    }

    if (CommonButton_Right)
    {
        CommonButton_Right->OnClicked().RemoveAll(this);
        CommonButton_Right->OnClicked().AddUObject(this, &ThisClass::HandleRightClicked);
    }

    if (CommonButton_Close)
    {
        CommonButton_Close->OnClicked().RemoveAll(this);
        CommonButton_Close->OnClicked().AddUObject(this, &ThisClass::HandleCloseClicked);
    }

    if (!SelectionChangedHandle.IsValid())
    {
        if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
        {
            SelectionChangedHandle = SelectionSubsystem->OnSelectionChanged.AddUObject(this, &ThisClass::HandleSelectionChanged);
        }
    }
}

void UEMRFrontendLobbyCharacterSelectionWidget::NativeOnActivated()
{
    Super::NativeOnActivated();

    BindInput();
    SpawnPreviewStageIfNeeded();
    if (AEMRLobbyCharacterPreviewStage* PreviewStage = CachedPreviewStage.Get())
    {
        PreviewStage->SetPreviewCaptureEnabled(true);
    }
    CurrentIndex = ResolveInitialIndex();
    RefreshPreview();
}

void UEMRFrontendLobbyCharacterSelectionWidget::NativeOnDeactivated()
{
    if (AEMRLobbyCharacterPreviewStage* PreviewStage = CachedPreviewStage.Get())
    {
        PreviewStage->SetPreviewCaptureEnabled(false);
        PreviewStage->ClearPreview();
    }

    Super::NativeOnDeactivated();
}

void UEMRFrontendLobbyCharacterSelectionWidget::NativeDestruct()
{
    UnbindInput();

    if (AEMRLobbyCharacterPreviewStage* PreviewStage = CachedPreviewStage.Get())
    {
        PreviewStage->SetPreviewCaptureEnabled(false);
    }
    DestroyPreviewStage();

    Super::NativeDestruct();
}

void UEMRFrontendLobbyCharacterSelectionWidget::OnBackBoundActionTriggered()
{
    DeactivateWidget();

    UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
    if (!UIManagerSubsystem)
    {
        return;
    }

    UEMRCommonPrimaryLayoutWidget* PrimaryLayout = UIManagerSubsystem->GetPrimaryLayoutWidget();
    if (!PrimaryLayout)
    {
        return;
    }

    UCommonActivatableWidgetContainerBase* FrontendStack = PrimaryLayout->FindWidgetStackByTag(EMRTags::UI::WidgetStack::Frontend);
    if (!FrontendStack)
    {
        return;
    }

    for (UCommonActivatableWidget* Widget : FrontendStack->GetWidgetList())
    {
        if (UEMRFrontendLobbyScreenWidget* LobbyScreen = Cast<UEMRFrontendLobbyScreenWidget>(Widget))
        {
            LobbyScreen->FocusCharactersButton();
            break;
        }
    }
}

void UEMRFrontendLobbyCharacterSelectionWidget::UnbindInput()
{
    if (CommonButton_Left)
    {
        CommonButton_Left->OnClicked().RemoveAll(this);
    }

    if (CommonButton_Right)
    {
        CommonButton_Right->OnClicked().RemoveAll(this);
    }

    if (CommonButton_Close)
    {
        CommonButton_Close->OnClicked().RemoveAll(this);
    }

    if (SelectionChangedHandle.IsValid())
    {
        if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
        {
            SelectionSubsystem->OnSelectionChanged.Remove(SelectionChangedHandle);
        }
        SelectionChangedHandle.Reset();
    }
}

void UEMRFrontendLobbyCharacterSelectionWidget::HandleLeftClicked()
{
    const int32 CharacterCount = GetCharacterCount();
    if (CharacterCount <= 0)
    {
        return;
    }

    CurrentIndex = (CurrentIndex - 1 + CharacterCount) % CharacterCount;

    if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
    {
        SelectionSubsystem->SetSelectionIndex(CurrentIndex);
    }

    RefreshPreview();
}

void UEMRFrontendLobbyCharacterSelectionWidget::HandleRightClicked()
{
    const int32 CharacterCount = GetCharacterCount();
    if (CharacterCount <= 0)
    {
        return;
    }

    CurrentIndex = (CurrentIndex + 1) % CharacterCount;

    if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
    {
        SelectionSubsystem->SetSelectionIndex(CurrentIndex);
    }

    RefreshPreview();
}

void UEMRFrontendLobbyCharacterSelectionWidget::HandleCloseClicked()
{
    DeactivateWidget();
}

void UEMRFrontendLobbyCharacterSelectionWidget::RefreshPreview()
{
    SpawnPreviewStageIfNeeded();

    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    const int32 CharacterCount = Catalog ? Catalog->Characters.Num() : 0;

    if (CommonButton_Left)
    {
        CommonButton_Left->SetIsEnabled(CharacterCount > 1);
    }

    if (CommonButton_Right)
    {
        CommonButton_Right->SetIsEnabled(CharacterCount > 1);
    }

    if (!Catalog || CharacterCount == 0)
    {
        static bool bWarnedMissingCatalog = false;
        if (!bWarnedMissingCatalog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LobbyCharacterSelection] Missing or empty lobby character catalog."));
            bWarnedMissingCatalog = true;
        }
        if (CommonTextBlock_CharacterName)
        {
            CommonTextBlock_CharacterName->SetText(FText::GetEmpty());
        }
        return;
    }

    if (!Catalog->Characters.IsValidIndex(CurrentIndex))
    {
        CurrentIndex = 0;
    }

    const FLobbyCharacterDefinition* Definition = Catalog->GetCharacterDefinition(CurrentIndex);
    if (Definition && CommonTextBlock_CharacterName)
    {
        CommonTextBlock_CharacterName->SetText(Definition->DisplayName);
    }

    if (AEMRLobbyCharacterPreviewStage* PreviewStage = CachedPreviewStage.Get())
    {
        UTextureRenderTarget2D* RenderTarget = PreviewStage->GetRenderTarget();
        const bool bNeedsNewTarget = !RenderTarget
            || RenderTarget->RenderTargetFormat != RTF_RGBA16f
            || RenderTarget->SizeX != 512
            || RenderTarget->SizeY != 512;

        if (bNeedsNewTarget && Image_CharacterPreview)
        {
            RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 512, 512, RTF_RGBA16f);
            RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
            RenderTarget->UpdateResourceImmediate(true);
        }

        if (RenderTarget)
        {
            PreviewStage->SetRenderTarget(RenderTarget);
        }

        PreviewStage->SetPreviewCaptureEnabled(true);
        PreviewStage->SetCharacterIndex(CurrentIndex);
        PreviewStage->RequestPreviewCapture();

        if (Image_CharacterPreview && RenderTarget)
        {
            if (PreviewMaterial)
            {
                if (!PreviewMaterialInstance)
                {
                    PreviewMaterialInstance = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
                    if (PreviewMaterialInstance)
                    {
                        Image_CharacterPreview->SetBrushFromMaterial(PreviewMaterialInstance);
                    }
                }

                if (PreviewMaterialInstance)
                {
                    PreviewMaterialInstance->SetTextureParameterValue(PreviewTextureParamName, RenderTarget);
                }
            }
            else
            {
                static bool bWarnedMissingPreviewMaterial = false;
                if (!bWarnedMissingPreviewMaterial)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[LobbyCharacterSelection] PreviewMaterial is not set; using raw render target."));
                    bWarnedMissingPreviewMaterial = true;
                }

                FSlateBrush Brush;
                Brush.SetResourceObject(RenderTarget);
                Image_CharacterPreview->SetBrush(Brush);
            }

            Image_CharacterPreview->SetColorAndOpacity(FLinearColor::White);
        }
    }
}

int32 UEMRFrontendLobbyCharacterSelectionWidget::ResolveInitialIndex() const
{
    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    const int32 CharacterCount = Catalog ? Catalog->Characters.Num() : 0;

    int32 IndexFromSubsystem = INDEX_NONE;
    if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
    {
        IndexFromSubsystem = SelectionSubsystem->GetCurrentSelectionIndex();
    }

    if (CharacterCount <= 0)
    {
        return IndexFromSubsystem;
    }

    if (Catalog->Characters.IsValidIndex(IndexFromSubsystem))
    {
        return IndexFromSubsystem;
    }

    return 0;
}

int32 UEMRFrontendLobbyCharacterSelectionWidget::GetCharacterCount() const
{
    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    return Catalog ? Catalog->Characters.Num() : 0;
}

void UEMRFrontendLobbyCharacterSelectionWidget::HandleSelectionChanged(int32 NewIndex)
{
    CurrentIndex = NewIndex;
    RefreshPreview();
}

void UEMRFrontendLobbyCharacterSelectionWidget::SpawnPreviewStageIfNeeded()
{
    if (CachedPreviewStage.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor*> FoundStages;
    UGameplayStatics::GetAllActorsOfClass(this, AEMRLobbyCharacterPreviewStage::StaticClass(), FoundStages);
    if (!FoundStages.IsEmpty())
    {
        if (AEMRLobbyCharacterPreviewStage* ExistingStage = Cast<AEMRLobbyCharacterPreviewStage>(FoundStages[0]))
        {
            ExistingStage->SetActorLocation(PreviewStageSpawnLocation);
            ExistingStage->SetActorEnableCollision(false);
            CachedPreviewStage = ExistingStage;
            return;
        }
    }

    TSubclassOf<AEMRLobbyCharacterPreviewStage> SpawnClass = PreviewStageClass;
    if (!SpawnClass)
    {
        SpawnClass = AEMRLobbyCharacterPreviewStage::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Owner = GetOwningPlayer();

    AEMRLobbyCharacterPreviewStage* SpawnedStage = World->SpawnActor<AEMRLobbyCharacterPreviewStage>(
        SpawnClass,
        PreviewStageSpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!SpawnedStage)
    {
        return;
    }

    SpawnedStage->SetActorEnableCollision(false);
    CachedPreviewStage = SpawnedStage;
}

void UEMRFrontendLobbyCharacterSelectionWidget::DestroyPreviewStage()
{
    if (AEMRLobbyCharacterPreviewStage* PreviewStage = CachedPreviewStage.Get())
    {
        PreviewStage->Destroy();
    }

    CachedPreviewStage.Reset();
}
