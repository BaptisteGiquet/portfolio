#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EMRFrontendPlayerController.generated.h"

enum class ELobbyInviteJoinResult : uint8;
class UEMRCommonPrimaryLayoutWidget;
class UInputMappingContext;
class UEnhancedInputUserSettings;

/**
 * 
 */
UCLASS()
class LOD_API AEMRFrontendPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEMRFrontendPlayerController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void ToggleLobbyReady();

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	void RequestJoinLobbyByInviteCode(const FString& Code);

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	void RequestRegenerateLobbyInviteCode();

private:
	void TryBootstrapFrontendUI();
	TSubclassOf<UEMRCommonPrimaryLayoutWidget> ResolveFrontendPrimaryLayoutWidgetClass() const;
	void RegisterControlsRemapInputMappingContext();
	const UInputMappingContext* ResolveControlsRemapInputMappingContext() const;

	UFUNCTION(Server, Reliable)
	void Server_RegenerateLobbyInviteCode();

	UFUNCTION()
	void HandlePostUserSettingsInitialized(const UEnhancedInputUserSettings* Settings);

	void HandleInviteJoinResult(ELobbyInviteJoinResult Result);

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
	TSoftClassPtr<UEMRCommonPrimaryLayoutWidget> FrontendPrimaryLayoutWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
	TSoftObjectPtr<UInputMappingContext> ControlsRemapInputMappingContext;

	bool bFrontendUIBootstrapped = false;
};
