#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Developer/EMRDeveloperToolsTypes.h"
#include "Interfaces/EMRGameplayEventInterface.h"
#include "Framework/EMRNightShiftGameState.h"
#include "TimerManager.h"
#include "EMRPlayerController.generated.h"


class UEMRMachineInteractionWidget;
class UEMRBaseMachineTaskWidget;
class AEMRPatient;
class UInputAction;
class UInputMappingContext;
class AEMRPlayerCharacter;
class UEMRNightShiftDefinition;
class UDEPRECATEDEMRNightShiftStatusWidget;
class UEMRCommonHubNightShiftSelectionWidget;
class UDEPRECATEDEMRNightShiftSummaryWidget;
class AEMRNightShiftGameState;
class UEMRCommonPrimaryLayoutWidget;
class UEMRFrontendCommonActivatableWidgetBase;
class UWidget;
class UUserWidget;
class UEMRExamQueueSubsystem;
class UEMRDeveloperToolsWidget;

UENUM(BlueprintType)
enum class EEMRReceptionValidationResult : uint8
{
	Success,
	WrongExamSelection,
	NoExamWaitingSeatAvailable,
	UnknownFailure
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnReceptionValidationResult, AEMRPatient*, EEMRReceptionValidationResult);

UCLASS()
class AEMRPlayerController : public APlayerController, public IGenericTeamAgentInterface, public IEMRGameplayEventInterface
{
    GENERATED_BODY()


public:
    AEMRPlayerController();

    // Only called on the server
    virtual void OnPossess(APawn* InPawn) override;

    // Only called on the client (or P2P listening server)
    virtual void AcknowledgePossession(class APawn* InPawn) override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    void ToggleGameplayMenu();
    void RequestReturnToFrontendWithNightShiftTelemetry();

    UFUNCTION(BlueprintCallable, Category = "EMR|Session|Invite")
    void RequestRegenerateSessionInviteCode();

	
    UFUNCTION(Server, Reliable)
    void Server_CompleteReceptionTask(AEMRPatient* Patient, const TArray<FGameplayTag>& Exams, bool bNoExamNeeded, bool bRegisterPatientFolder);

    UFUNCTION(Client, Reliable)
    void Client_NotifyReceptionValidationResult(AEMRPatient* Patient, EEMRReceptionValidationResult Result);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveNightShiftTelemetry(const FEMRNightShiftTelemetryRecord& Record);

    UFUNCTION(Server, Reliable)
    void Server_RequestNightShiftTelemetryOnLeave();

    UFUNCTION(Client, Reliable)
    void Client_ConfirmReturnToFrontendAfterTelemetry();

    FOnReceptionValidationResult OnReceptionValidationResult;

    UFUNCTION(Server, Reliable)
    void Server_SelectNightShiftByIndex(int32 OfferIndex);

    UFUNCTION(Server, Reliable)
    void Server_VoteForHubUpgradeByIndex(int32 OfferIndex);

    UFUNCTION(Server, Reliable)
    void Server_RequestStartHubUpgradeVoteNow();

    UFUNCTION(Server, Reliable)
    void Server_RegenerateSessionInviteCode();

    UFUNCTION(Server, Reliable)
    void Server_RequestDeveloperToolAction(const FEMRDeveloperToolActionRequest& Request);

    UFUNCTION(Server, Reliable)
    void Server_RequestDeveloperToolUpgradeOptions();

    UFUNCTION(Client, Reliable)
    void Client_ReceiveDeveloperToolActionResult(const FEMRDeveloperToolActionResult& Result);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveDeveloperToolUpgradeOptions(const TArray<FEMRDeveloperToolUpgradeOption>& Options);

    void ToggleDeveloperTools();

private:
    bool DoesReceptionSelectionMatchRequiredExams(UEMRExamQueueSubsystem* ExamQueue, AEMRPatient* Patient, const TArray<FGameplayTag>& Exams, bool bNoExamNeeded) const;
    void HandleNightShiftTelemetryPublished(const FEMRNightShiftTelemetryRecord& Record);
    void ContinueReturnToFrontendAfterTelemetry();
    void HandleReturnToFrontendTelemetryTimeout();

    void SpawnHUDWidgets();
    void CleanupHubUI();
    void BindToRunState();
    void UnbindFromRunState();
    void HandleRunPhaseChanged(EER_RunPhase /*NewPhase*/, EER_RunPhase /*PreviousPhase*/);
	void EnsureReticleWidget();
    void ShowGameplayMenu();
    void HideGameplayMenu();
    UEMRFrontendCommonActivatableWidgetBase* GetActiveGameMenuWidget() const;
    void ShowDeveloperTools();
    void HideDeveloperTools();
    UEMRDeveloperToolsWidget* GetActiveDeveloperToolsWidget() const;
    void NotifyDeveloperToolResult_Local(const FEMRDeveloperToolActionResult& Result);
    bool ExecuteDeveloperToolAction_Server(const FEMRDeveloperToolActionRequest& Request, FEMRDeveloperToolActionResult& OutResult);
    bool ApplyDeveloperEconomyDelta_Server(float RevenueDelta, float ReputationDelta, FString& OutError) const;
    bool ApplyDeveloperTimeDilationScale_Server(float Multiplier, FString& OutMessage) const;
    bool CollectDeveloperUpgradeOptions_Server(TArray<FEMRDeveloperToolUpgradeOption>& OutOptions, FString& OutError) const;
    void EnsureGameHudRootWidget();
    TSubclassOf<UEMRCommonPrimaryLayoutWidget> ResolvePrimaryLayoutWidgetClass();


    bool IsInHubPhase() const;



    UFUNCTION()
    virtual bool SendGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;

    UFUNCTION()
    virtual bool SendGameplayEventToActor(AActor* TargetActor, FGameplayTag EventTag, const FGameplayEventData& Payload) override;


    UPROPERTY()
    TObjectPtr<AEMRPlayerCharacter> PlayerCharacter = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
	TSoftClassPtr<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutWidgetClass;
	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> NightShiftSelectionWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI", meta = (Categories = "EMR.UI.WidgetStack"))
	FGameplayTag WidgetStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag GameMenuStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> GameMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag GameHudStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> GameHudRootWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|DevTools", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag DeveloperToolsStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|DevTools")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> DeveloperToolsWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|DevTools")
    int32 DeveloperToolsWidgetZOrder = 200;

    UPROPERTY(Transient)
    TObjectPtr<UEMRFrontendCommonActivatableWidgetBase> ActiveGameHudRootWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEMRFrontendCommonActivatableWidgetBase> ActiveGameMenuWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEMRDeveloperToolsWidget> ActiveDeveloperToolsWidget = nullptr;

    int32 GameMenuPushId = 0;
    int32 DeveloperToolsPushId = 0;


    UPROPERTY()
    TObjectPtr<UEMRCommonHubNightShiftSelectionWidget> NightShiftSelectionWidget = nullptr;

   
	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
	TSubclassOf<UUserWidget> ReticleWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
	int32 ReticleZOrder = 100;

	UPROPERTY()
	TObjectPtr<UUserWidget> ReticleWidget = nullptr;
	
	
    bool bIsTraveling = false;
    bool bNightShiftWidgetPushPending = false;
    bool bIgnoreNightShiftWidgetPush = false;
    bool bPendingReturnToFrontendAfterTelemetry = false;
    int32 NightShiftWidgetPushId = 0;

    mutable TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
    FDelegateHandle RunPhaseChangedHandle;

	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "EMR|UI", meta = (AllowPrivateAccess = true))
	TObjectPtr<UEMRCommonPrimaryLayoutWidget> CachedPrimaryLayoutWidget;


    /**************************/
    /*********PATIENT**********/
    /**************************/


protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnRep_PlayerState() override;
    virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandlePendingTravelUpdated();

    FDelegateHandle PendingTravelDelegateHandle;
    FDelegateHandle NightShiftTelemetryPublishedHandle;
    FTimerHandle ReturnToFrontendTelemetryTimeoutHandle;

public:
    UFUNCTION(Server, Reliable)
    void Server_CallWaitingPatient(AEMRPatient* Patient);
};
