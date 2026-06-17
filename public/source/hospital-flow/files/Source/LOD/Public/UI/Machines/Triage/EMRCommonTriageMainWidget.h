
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Common/EMRPreferredFocusWidgetInterface.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRCommonTriageMainWidget.generated.h"

class UCommonTileView;
class UEMRWaitingRoomManagerComponent;
class UEMRTriagePatientCardObject;
class UEMRWaitingSeatComponent;
class UUserWidget;
class UWidget;
class UEMRFrontendCommonButtonBase;
class AActor;
class UWidgetComponent;
class UEMRTriagePatientDetailWidget;
class AEMRReceptionMachine;

DECLARE_MULTICAST_DELEGATE(FOnTriageWidgetDeactivated);
DECLARE_MULTICAST_DELEGATE(FOnTriageExitRequested);

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonTriageMainWidget : public UEMRFrontendCommonActivatableWidgetBase, public IEMRPreferredFocusWidgetInterface
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual UWidget* GetPreferredFocusWidget_Implementation() const override;

    FOnTriageWidgetDeactivated OnTriageWidgetDeactivated;
    FOnTriageExitRequested OnTriageExitRequested;

private:
	void RefreshPatientCards();
	void ClearPatientCards();
	UEMRWaitingRoomManagerComponent* ResolveWaitingRoomManager();

	UFUNCTION()
	void OnSeatAssigned(UEMRWaitingSeatComponent* Seat, AActor* PatientActor);

	UFUNCTION()
	void OnSeatReleased(UEMRWaitingSeatComponent* Seat);

	UFUNCTION()
	void OnPatientCalled(AActor* PatientActor);

	UFUNCTION()
	void HandleWaitingPatientsChanged();

	UFUNCTION()
	void HandlePatientPhaseChanged(FGameplayTag NewPhase);

    void HandleEntryWidgetReleased(UUserWidget& Widget);
    void HandleExitClicked();
    void HandlePatientClicked(UObject* Item);
    void HandlePanelCloseRequested();
    AEMRReceptionMachine* ResolveReceptionMachine();

    //***** Bound Widgets *****//
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonTileView> CommonTileView_PatientCards;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Exit = nullptr;
    //***** Bound Widgets *****//

    UPROPERTY()
    TArray<TObjectPtr<UEMRTriagePatientCardObject>> PatientCardObjects;

    UPROPERTY()
    TWeakObjectPtr<UEMRWaitingRoomManagerComponent> WaitingRoomManager;

    TWeakObjectPtr<AEMRReceptionMachine> CachedReceptionMachine;
    TWeakObjectPtr<UWidgetComponent> CachedPanelWidgetComponent;
    TWeakObjectPtr<UEMRTriagePatientDetailWidget> CachedPanelWidget;
};
