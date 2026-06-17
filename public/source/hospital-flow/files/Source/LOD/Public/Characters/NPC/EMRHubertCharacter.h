#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EMRHubertCharacter.generated.h"

class USoundBase;
class UEMRHubertAnimInstance;

UENUM(BlueprintType)
enum class EEMRHubertState : uint8
{
    HubDriver UMETA(DisplayName = "Hub Driver"),
    NightPatrol UMETA(DisplayName = "Night Patrol"),
    WindowWatch UMETA(DisplayName = "Window Watch"),
    ChaseEscaper UMETA(DisplayName = "Chase Escaper"),
    GrabEscaper UMETA(DisplayName = "Grab Escaper"),
    CarryEscaper UMETA(DisplayName = "Carry Escaper"),
    ThrowEscaper UMETA(DisplayName = "Throw Escaper"),
    ReturnEscaper UMETA(DisplayName = "Return Escaper")
};

UCLASS()
class LOD_API AEMRHubertCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEMRHubertCharacter();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetHubertState(EEMRHubertState NewState);

    EEMRHubertState GetHubertState() const { return HubertState; }

    void PlayVoiceLineForAllPlayers(USoundBase* VoiceLine);

protected:
    UFUNCTION()
    void OnRep_HubertState();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayVoiceLineAtHubert(USoundBase* VoiceLine);

private:
    void PushStateToAnimInstance() const;

private:
    UPROPERTY(ReplicatedUsing = OnRep_HubertState, VisibleAnywhere, Category = "EMR|Hubert|State")
    EEMRHubertState HubertState = EEMRHubertState::HubDriver;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Animation")
    TSubclassOf<UEMRHubertAnimInstance> HubertAnimInstanceClass;
};
