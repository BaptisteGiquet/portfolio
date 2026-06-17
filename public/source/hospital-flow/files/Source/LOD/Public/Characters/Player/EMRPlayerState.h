#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "EMRPlayerState.generated.h"

class UGameplayEffect;
class UEMRAbilitySystemComponent;
class UEMRPatientAttributeSet;
class UEMRTeamAttributeSet;

USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FText PlayerName = FText::GetEmpty();

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	bool bIsPlayerReady = false;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 SelectedCharacterIndex = INDEX_NONE;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyPlayerInfoChanged, const FLobbyPlayerInfo&, LobbyPlayerInfo);

UCLASS()
class AEMRPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEMRPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	void InitializeAbilitySystemForPawn(APawn* InPawn);

	void TryInitAbilityActorInfo();

	TSubclassOf<UGameplayEffect> GetAddTagsToPlayerEffect() const { return AddTagsToPlayerEffect; };
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	FLobbyPlayerInfo GetLobbyPlayerInfo() const { return LobbyPlayerInfo; }

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	bool IsLobbyReady() const { return LobbyPlayerInfo.bIsPlayerReady; }

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLobbyReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	int32 GetLobbyCharacterIndex() const { return LobbyPlayerInfo.SelectedCharacterIndex; }

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLobbyCharacterIndex(int32 NewIndex);

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyPlayerInfoChanged OnLobbyPlayerInfoChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EMR|GAS", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UEMRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(ReplicatedUsing = OnRep_AbilityActorInfoReady)
	bool bAbilityActorInfoReady = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|GAS", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayEffect> AddTagsToPlayerEffect;
	
	UFUNCTION()
	void OnRep_AbilityActorInfoReady();

	UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayerInfo)
	FLobbyPlayerInfo LobbyPlayerInfo;

	UFUNCTION()
	void OnRep_LobbyPlayerInfo();

	UFUNCTION(Server, Reliable)
	void Server_SetLobbyReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void Server_SetLobbyCharacterIndex(int32 NewIndex);
	
	UFUNCTION(Server, Reliable)
	void Server_UpdateLobbyPlayerInfo();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void UpdateLobbyPlayerInfo();

	void NotifyLobbyPlayerInfoChanged();
};
