#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRFrontendPlayerSlotInfosWidget.generated.h"

class APlayerState;
class UEMRFrontendCommonButtonBase;
class UCommonTextBlock;

UCLASS()
class LOD_API UEMRFrontendPlayerSlotInfosWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetPlayerInfo(const FText& PlayerName, bool bIsReady);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ClearPlayerInfo();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetAssignedPlayerState(APlayerState* InPlayerState);

protected:
    virtual void NativeOnInitialized() override;

private:
    void RefreshKickVisibility();

    UFUNCTION()
    void HandleKickClicked();

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_PlayerName;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_ReadyStatus;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Kick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Lobby|Kick")
    FText KickConfirmTitle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Lobby|Kick")
    FText KickConfirmMessage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    FText ReadyText;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
    FText NotReadyText;

    TWeakObjectPtr<APlayerState> AssignedPlayerState;
};
