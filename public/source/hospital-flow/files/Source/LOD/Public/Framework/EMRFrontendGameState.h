#pragma once

#include "CoreMinimal.h"
#include "Framework/EMRLobbyGameState.h"
#include "EMRFrontendGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrontendLobbyHostingChanged, bool);

UCLASS()
class LOD_API AEMRFrontendGameState : public AEMRLobbyGameState
{
	GENERATED_BODY()

public:
	AEMRFrontendGameState();

	FOnFrontendLobbyHostingChanged OnLobbyHostingChanged;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	bool IsLobbyHosting() const { return bIsLobbyHosting; }

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLobbyHosting(bool bHosting);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_LobbyHosting();

	UPROPERTY(ReplicatedUsing = OnRep_LobbyHosting, BlueprintReadOnly, Category = "Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsLobbyHosting = false;
};
