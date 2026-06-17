#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ProximityVoiceTypes.h"
#include "ProximityVoiceLocalPlayerSubsystem.generated.h"

class APlayerState;
class IOnlineVoice;
class UProximityVoiceTalkerComponent;
class USceneComponent;

UCLASS()
class PROXIMITYVOICERUNTIME_API UProximityVoiceLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    void ApplyVoiceSettings(const FString& InMicrophoneMode, float InMicrophoneVolume, float InProximityChatVolume, bool bInMicMonitorEnabled, const FString& InPreferredInputDeviceId);

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    void SetPushToTalkActive(bool bPressed);

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    void SetRemotePlayerVolume(const FString& RemotePlayerId, float InVolume);

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    void SetRemotePlayerMuted(const FString& RemotePlayerId, bool bMuted);

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    void SetMicMonitorEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "EMR|Voice")
    bool SetPreferredInputDevice(const FString& DeviceId);

    UFUNCTION(BlueprintPure, Category = "EMR|Voice")
    TArray<FProximityVoiceInputDevice> GetAvailableInputDevices() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Voice")
    TArray<FProximityVoiceRemotePlayerView> GetRemotePlayersView() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Voice")
    bool IsLocalMicActive() const { return bLocalMicActive; }

    UFUNCTION(BlueprintPure, Category = "EMR|Voice")
    bool IsPushToTalkMode() const { return bPushToTalkMode; }

    UFUNCTION(BlueprintPure, Category = "EMR|Voice")
    FString GetPreferredInputDeviceId() const { return PreferredInputDeviceId; }

    static TArray<FProximityVoiceInputDevice> QuerySystemInputDevices();

    FOnProximityVoiceRosterChanged& OnVoiceRosterChanged() { return OnVoiceRosterChangedNative; }
    FOnProximityVoiceRemoteTalkingChanged& OnRemoteTalkingStateChanged() { return OnRemoteTalkingStateChangedNative; }
    FOnProximityVoiceLocalMicActivityChanged& OnLocalMicActivityChanged() { return OnLocalMicActivityChangedNative; }

    UPROPERTY(BlueprintAssignable, Category = "EMR|Voice")
    FOnProximityVoiceRosterChangedDynamic OnVoiceRosterChangedDynamic;

    UPROPERTY(BlueprintAssignable, Category = "EMR|Voice")
    FOnProximityVoiceRemoteTalkingChangedDynamic OnRemoteTalkingStateChangedDynamic;

    UPROPERTY(BlueprintAssignable, Category = "EMR|Voice")
    FOnProximityVoiceLocalMicActivityChangedDynamic OnLocalMicActivityChangedDynamic;

private:
    struct FRemoteTalkerRuntimeData
    {
        TWeakObjectPtr<APlayerState> PlayerState;
        TWeakObjectPtr<UProximityVoiceTalkerComponent> Talker;
        TSharedPtr<const FUniqueNetId> UniqueNetId;
        float LastAmplitude = 0.0f;
        int32 LastPingMs = INDEX_NONE;
        bool bWasTalking = false;
    };

    void HandlePollTick();
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void EnsurePollTimerBoundToCurrentWorld();
    void ResetVoiceStateAfterWorldTransition();
    void EnsureLocalTalkerRegistration();
    void UpdateNetworkedVoiceState();
    void RefreshRemoteTalkers();
    void UpdateTalkingStateAndVolumes();

    void ApplyVoiceSettingsToTalker(APlayerState* PlayerState, UProximityVoiceTalkerComponent* Talker) const;
    USceneComponent* ResolveAttachComponent(const APlayerState* PlayerState) const;
    APlayerState* ResolveLocalPlayerState() const;

    FString GetRemotePlayerId(const APlayerState* PlayerState) const;
    TSharedPtr<const FUniqueNetId> GetRemoteUniqueNetId(const APlayerState* PlayerState) const;

    FProximityVoiceRemotePreference GetPreferenceFor(const FString& RemotePlayerId) const;
    void SavePreferences() const;
    void LoadPreferences();
    FString GetSaveSlotName() const;

    IOnlineVoicePtr ResolveVoiceInterface() const;
    uint8 GetLocalUserNum() const;

    bool ApplyPreferredInputDeviceBridge();
    bool ResolveInputDeviceName(const FString& DeviceId, FString& OutDeviceName) const;

    void SetLocalMicActive(bool bInLocalMicActive);

    TMap<FString, FRemoteTalkerRuntimeData> RemoteTalkersById;
    TMap<FString, FProximityVoiceRemotePreference> RemotePreferencesByPlayerId;
    int32 CachedLocalPingMs = INDEX_NONE;

    mutable TWeakObjectPtr<class USoundAttenuation> CachedVoiceAttenuation;
    mutable TWeakObjectPtr<class USoundEffectSourcePresetChain> CachedVoiceSourceEffects;

    FTimerHandle PollTimerHandle;
    TWeakObjectPtr<UWorld> PollTimerWorld;
    FDelegateHandle PostLoadMapHandle;
    float PollIntervalSeconds = 0.2f;

    FOnProximityVoiceRosterChanged OnVoiceRosterChangedNative;
    FOnProximityVoiceRemoteTalkingChanged OnRemoteTalkingStateChangedNative;
    FOnProximityVoiceLocalMicActivityChanged OnLocalMicActivityChangedNative;

    FString PreferredInputDeviceId;
    float GlobalRemoteVoiceVolume = 1.0f;
    float LocalMicInputGain = 1.0f;
    bool bMicMonitorEnabled = false;
    bool bPushToTalkMode = true;
    bool bPushToTalkPressed = false;
    bool bLocalMicActive = false;
    bool bLocalTalkerRegistered = false;
    bool bNetworkedVoiceActive = false;
    bool bPendingInputDeviceApply = false;
    bool bForceRemoteTalkerReregistration = false;
};
