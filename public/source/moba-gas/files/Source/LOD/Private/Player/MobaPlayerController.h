
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaPlayerController.generated.h"


class UInputAction;
class UInputMappingContext;
class UMobaGameplayWidget;
class AMobaPlayerCharacter;



UCLASS()
class AMobaPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
public:
	// Only called on the server
	virtual void OnPossess(APawn* InPawn) override;
	
	// Only called on the client (or P2P listening server)
	virtual void AcknowledgePossession(class APawn* InPawn) override;

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetupInputComponent() override;

	void MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam);

private:
	void SpawnGameplayWidget();
	void ShowWinLoseState();

	UFUNCTION()
	void ToggleShopVisibility();

	UFUNCTION()
	void ToggleGameplayMenu();

	UFUNCTION()
	void OnRep_bHasWonMatch();

	UPROPERTY(EditDefaultsOnly, Category = "Tag")
	FGameplayTag PlayerControllerTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMobaGameplayWidget> GameplayWidgetClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Essential Variables")
	TObjectPtr<AMobaPlayerCharacter> PlayerCharacter = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UMobaGameplayWidget> GameplayWidget = nullptr;

	UPROPERTY(Replicated)
	FGenericTeamId TeamID;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UIInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleShopInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleGameplayMenuInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "ViewTarget")
	float MatchFinishedBlendTimeDuration = 2.f;

	UPROPERTY(ReplicatedUsing = OnRep_bHasWonMatch)
	bool bHasWonMatch = false;
};
