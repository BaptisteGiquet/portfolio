#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRFrontendLobbyCharacterSelectionWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UEMRFrontendCommonButtonBase;
class AEMRLobbyCharacterPreviewStage;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendLobbyCharacterSelectionWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

private:
    void BindInput();
    void UnbindInput();
    void OnBackBoundActionTriggered();
    void HandleLeftClicked();
    void HandleRightClicked();
    void HandleCloseClicked();
    void RefreshPreview();
    int32 ResolveInitialIndex() const;
    int32 GetCharacterCount() const;
    void HandleSelectionChanged(int32 NewIndex);
    void SpawnPreviewStageIfNeeded();
    void DestroyPreviewStage();

    //***** Bound Widgets *****//
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Left = nullptr;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Right = nullptr;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Close = nullptr;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_CharacterName = nullptr;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> Image_CharacterPreview = nullptr;
    //***** Bound Widgets *****//

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|Lobby")
    TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

    static const FName PreviewTextureParamName;

    int32 CurrentIndex = INDEX_NONE;

    TWeakObjectPtr<AEMRLobbyCharacterPreviewStage> CachedPreviewStage;
    FDelegateHandle SelectionChangedHandle;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance = nullptr;

    FUIActionBindingHandle BackActionHandle;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Characters")
    TSubclassOf<AEMRLobbyCharacterPreviewStage> PreviewStageClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Characters")
    FVector PreviewStageSpawnLocation = FVector(0.f, 0.f, -100000.f);
};
