#include "ProximityVoiceLocalPlayerSubsystem.h"

#include "AudioCaptureCore.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/VoiceInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/VoiceConfig.h"
#include "Online.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "ProximityVoiceDeveloperSettings.h"
#include "ProximityVoiceSaveGame.h"
#include "ProximityVoiceTalkerComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundEffectSource.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

#define protected public
#include "VoiceEngineImpl.h"
#include "VoiceInterfaceImpl.h"
#undef protected

namespace
{
    constexpr TCHAR DefaultInputDeviceLabel[] = TEXT("System Default");

    template <typename Tag, typename Tag::type Member>
    struct TPrivateMemberAccessor
    {
        friend typename Tag::type AccessMember(Tag)
        {
            return Member;
        }
    };

    struct FVoiceCaptureMemberTag
    {
        using type = TSharedPtr<IVoiceCapture> FVoiceEngineImpl::*;
        friend type AccessMember(FVoiceCaptureMemberTag);
    };

    template struct TPrivateMemberAccessor<FVoiceCaptureMemberTag, &FVoiceEngineImpl::VoiceCapture>;

    FVoiceEngineImpl* ResolveVoiceEngineImpl(const IOnlineVoicePtr& VoiceInterface)
    {
        if (!VoiceInterface.IsValid())
        {
            return nullptr;
        }

        FOnlineVoiceImpl* VoiceImpl = static_cast<FOnlineVoiceImpl*>(VoiceInterface.Get());
        if (!VoiceImpl || !VoiceImpl->VoiceEngine.IsValid())
        {
            return nullptr;
        }

        return static_cast<FVoiceEngineImpl*>(VoiceImpl->VoiceEngine.Get());
    }

    TSharedPtr<IVoiceCapture> ResolveVoiceCapture(FVoiceEngineImpl* VoiceEngine)
    {
        if (!VoiceEngine)
        {
            return nullptr;
        }

        const FVoiceCaptureMemberTag::type VoiceCaptureMember = AccessMember(FVoiceCaptureMemberTag{});
        return VoiceEngine->*VoiceCaptureMember;
    }

    float ComputeDistanceAttenuation(const FVector& ListenerLocation, const FVector& SourceLocation, float FullVolumeDistance, float MaxDistance)
    {
        const float SafeFull = FMath::Max(0.0f, FullVolumeDistance);
        const float SafeMax = FMath::Max(SafeFull + 1.0f, MaxDistance);

        const float Distance = FVector::Dist(ListenerLocation, SourceLocation);
        if (Distance <= SafeFull)
        {
            return 1.0f;
        }

        if (Distance >= SafeMax)
        {
            return 0.0f;
        }

        const float Alpha = (Distance - SafeFull) / (SafeMax - SafeFull);
        return 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
    }

    int32 ResolveRoundedPingMs(const APlayerState* PlayerState)
    {
        if (!PlayerState)
        {
            return INDEX_NONE;
        }

        const float PingMs = PlayerState->GetPingInMilliseconds();
        if (!FMath::IsFinite(PingMs) || PingMs <= 0.0f)
        {
            return INDEX_NONE;
        }

        return FMath::RoundToInt(PingMs);
    }
}

void UProximityVoiceLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        if (World->GetNetMode() == NM_DedicatedServer)
        {
            return;
        }
    }

    LoadPreferences();
    bPendingInputDeviceApply = !PreferredInputDeviceId.IsEmpty();

    const UProximityVoiceDeveloperSettings* VoiceSettings = GetDefault<UProximityVoiceDeveloperSettings>();
    PollIntervalSeconds = VoiceSettings ? FMath::Max(0.05f, VoiceSettings->PollIntervalSeconds) : 0.2f;

    if (!PostLoadMapHandle.IsValid())
    {
        PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
    }

    EnsurePollTimerBoundToCurrentWorld();
    HandlePollTick();
}

void UProximityVoiceLocalPlayerSubsystem::Deinitialize()
{
    if (PostLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        PostLoadMapHandle.Reset();
    }

    if (UWorld* BoundWorld = PollTimerWorld.Get())
    {
        BoundWorld->GetTimerManager().ClearTimer(PollTimerHandle);
        PollTimerWorld.Reset();
    }
    else if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PollTimerHandle);
    }

    if (IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface())
    {
        const uint8 LocalUserNum = GetLocalUserNum();
        if (bNetworkedVoiceActive)
        {
            VoiceInterface->StopNetworkedVoice(LocalUserNum);
            bNetworkedVoiceActive = false;
        }

        if (bLocalTalkerRegistered)
        {
            VoiceInterface->UnregisterLocalTalker(LocalUserNum);
            bLocalTalkerRegistered = false;
        }

        VoiceInterface->DisconnectAllEndpoints();
    }

    for (TPair<FString, FRemoteTalkerRuntimeData>& Pair : RemoteTalkersById)
    {
        if (UProximityVoiceTalkerComponent* Talker = Pair.Value.Talker.Get())
        {
            Talker->DestroyComponent();
        }
    }
    RemoteTalkersById.Empty();

    Super::Deinitialize();
}

void UProximityVoiceLocalPlayerSubsystem::ApplyVoiceSettings(const FString& InMicrophoneMode, float InMicrophoneVolume, float InProximityChatVolume, bool bInMicMonitorEnabled, const FString& InPreferredInputDeviceId)
{
    bPushToTalkMode = InMicrophoneMode.Equals(TEXT("PushToTalk"), ESearchCase::IgnoreCase);
    LocalMicInputGain = FMath::Clamp(InMicrophoneVolume, 0.0f, 1.0f);
    GlobalRemoteVoiceVolume = FMath::Clamp(InProximityChatVolume, 0.0f, 1.0f);

    if (IConsoleVariable* MicInputGainCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicInputGain")))
    {
        MicInputGainCVar->Set(LocalMicInputGain, ECVF_SetByGameSetting);
    }

    if (const UProximityVoiceDeveloperSettings* VoiceSettings = GetDefault<UProximityVoiceDeveloperSettings>())
    {
        if (IConsoleVariable* SilenceThresholdCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("voice.SilenceDetectionThreshold")))
        {
            SilenceThresholdCVar->Set(VoiceSettings->MicActivityThreshold, ECVF_SetByGameSetting);
        }
    }

    SetMicMonitorEnabled(bInMicMonitorEnabled);
    SetPreferredInputDevice(InPreferredInputDeviceId);

    for (TPair<FString, FRemoteTalkerRuntimeData>& Pair : RemoteTalkersById)
    {
        if (UProximityVoiceTalkerComponent* Talker = Pair.Value.Talker.Get())
        {
            Talker->SetGlobalVoiceVolume(GlobalRemoteVoiceVolume);
        }
    }

    UpdateNetworkedVoiceState();
}

void UProximityVoiceLocalPlayerSubsystem::SetPushToTalkActive(bool bPressed)
{
    if (bPushToTalkPressed == bPressed)
    {
        return;
    }

    bPushToTalkPressed = bPressed;
    UpdateNetworkedVoiceState();
}

void UProximityVoiceLocalPlayerSubsystem::SetRemotePlayerVolume(const FString& RemotePlayerId, float InVolume)
{
    if (RemotePlayerId.IsEmpty())
    {
        return;
    }

    FProximityVoiceRemotePreference& Preference = RemotePreferencesByPlayerId.FindOrAdd(RemotePlayerId);
    Preference.Volume = FMath::Clamp(InVolume, 0.0f, 2.0f);

    if (const FRemoteTalkerRuntimeData* RuntimeData = RemoteTalkersById.Find(RemotePlayerId))
    {
        if (UProximityVoiceTalkerComponent* Talker = RuntimeData->Talker.Get())
        {
            Talker->SetRemotePreference(Preference);
        }
    }

    SavePreferences();
    OnVoiceRosterChangedNative.Broadcast();
    OnVoiceRosterChangedDynamic.Broadcast();
}

void UProximityVoiceLocalPlayerSubsystem::SetRemotePlayerMuted(const FString& RemotePlayerId, bool bMuted)
{
    if (RemotePlayerId.IsEmpty())
    {
        return;
    }

    FProximityVoiceRemotePreference& Preference = RemotePreferencesByPlayerId.FindOrAdd(RemotePlayerId);
    Preference.bMuted = bMuted;

    if (const FRemoteTalkerRuntimeData* RuntimeData = RemoteTalkersById.Find(RemotePlayerId))
    {
        if (UProximityVoiceTalkerComponent* Talker = RuntimeData->Talker.Get())
        {
            Talker->SetRemotePreference(Preference);
        }

        if (IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface())
        {
            if (RuntimeData->UniqueNetId.IsValid())
            {
                const uint8 LocalUserNum = GetLocalUserNum();
                if (bMuted)
                {
                    VoiceInterface->MuteRemoteTalker(LocalUserNum, *RuntimeData->UniqueNetId, false);
                }
                else
                {
                    VoiceInterface->UnmuteRemoteTalker(LocalUserNum, *RuntimeData->UniqueNetId, false);
                }
            }
        }
    }

    SavePreferences();
    OnVoiceRosterChangedNative.Broadcast();
    OnVoiceRosterChangedDynamic.Broadcast();
}

void UProximityVoiceLocalPlayerSubsystem::SetMicMonitorEnabled(bool bEnabled)
{
    bMicMonitorEnabled = bEnabled;

    if (IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface())
    {
        if (bMicMonitorEnabled)
        {
            VoiceInterface->PatchLocalTalkerOutputToEndpoint(TEXT(""));
        }
        else
        {
            VoiceInterface->DisconnectAllEndpoints();
        }
    }
}

bool UProximityVoiceLocalPlayerSubsystem::SetPreferredInputDevice(const FString& DeviceId)
{
    PreferredInputDeviceId = DeviceId;
    bPendingInputDeviceApply = true;
    return ApplyPreferredInputDeviceBridge();
}

TArray<FProximityVoiceInputDevice> UProximityVoiceLocalPlayerSubsystem::GetAvailableInputDevices() const
{
    return QuerySystemInputDevices();
}

TArray<FProximityVoiceRemotePlayerView> UProximityVoiceLocalPlayerSubsystem::GetRemotePlayersView() const
{
    TArray<FProximityVoiceRemotePlayerView> Views;
    Views.Reserve(RemoteTalkersById.Num() + 1);

    for (const TPair<FString, FRemoteTalkerRuntimeData>& Pair : RemoteTalkersById)
    {
        const FRemoteTalkerRuntimeData& RuntimeData = Pair.Value;

        FProximityVoiceRemotePlayerView& NewView = Views.AddDefaulted_GetRef();
        NewView.PlayerId = Pair.Key;
        NewView.Preference = GetPreferenceFor(Pair.Key);
        NewView.bIsTalking = RuntimeData.bWasTalking;
        NewView.VoiceLevel = RuntimeData.LastAmplitude;
        NewView.PingMs = RuntimeData.LastPingMs;
        NewView.bIsLocalPlayer = false;

        const APlayerState* PlayerState = RuntimeData.PlayerState.Get();
        NewView.DisplayName = PlayerState ? FText::FromString(PlayerState->GetPlayerName()) : FText::FromString(Pair.Key);
    }

    if (APlayerState* LocalPlayerState = ResolveLocalPlayerState())
    {
        FProximityVoiceRemotePlayerView& LocalView = Views.AddDefaulted_GetRef();
        LocalView.PlayerId = GetRemotePlayerId(LocalPlayerState);
        LocalView.DisplayName = FText::FromString(LocalPlayerState->GetPlayerName());
        LocalView.bIsTalking = bLocalMicActive;
        LocalView.VoiceLevel = 0.0f;
        LocalView.PingMs = CachedLocalPingMs;
        LocalView.bIsLocalPlayer = true;
        LocalView.Preference = FProximityVoiceRemotePreference{};
    }

    Views.Sort([](const FProximityVoiceRemotePlayerView& A, const FProximityVoiceRemotePlayerView& B)
    {
        return A.DisplayName.CompareTo(B.DisplayName) < 0;
    });

    return Views;
}

TArray<FProximityVoiceInputDevice> UProximityVoiceLocalPlayerSubsystem::QuerySystemInputDevices()
{
    TArray<FProximityVoiceInputDevice> OutDevices;

    FProximityVoiceInputDevice& DefaultDevice = OutDevices.AddDefaulted_GetRef();
    DefaultDevice.DeviceId = TEXT("");
    DefaultDevice.DisplayName = FText::FromString(DefaultInputDeviceLabel);

    Audio::FAudioCapture AudioCapture;
    TArray<Audio::FCaptureDeviceInfo> DeviceInfos;
    AudioCapture.GetCaptureDevicesAvailable(DeviceInfos);

    for (const Audio::FCaptureDeviceInfo& Info : DeviceInfos)
    {
        if (Info.DeviceId.IsEmpty())
        {
            continue;
        }

        FProximityVoiceInputDevice& Device = OutDevices.AddDefaulted_GetRef();
        Device.DeviceId = Info.DeviceId;
        Device.DisplayName = FText::FromString(Info.DeviceName.IsEmpty() ? Info.DeviceId : Info.DeviceName);
    }

    return OutDevices;
}

void UProximityVoiceLocalPlayerSubsystem::HandlePollTick()
{
    EnsurePollTimerBoundToCurrentWorld();
    EnsureLocalTalkerRegistration();

    if (bPendingInputDeviceApply)
    {
        ApplyPreferredInputDeviceBridge();
    }

    RefreshRemoteTalkers();
    UpdateTalkingStateAndVolumes();
}

void UProximityVoiceLocalPlayerSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    EnsurePollTimerBoundToCurrentWorld();
    ResetVoiceStateAfterWorldTransition();
    HandlePollTick();
}

void UProximityVoiceLocalPlayerSubsystem::EnsurePollTimerBoundToCurrentWorld()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    UWorld* BoundWorld = PollTimerWorld.Get();
    if (BoundWorld == World && World->GetTimerManager().IsTimerActive(PollTimerHandle))
    {
        return;
    }

    if (BoundWorld)
    {
        BoundWorld->GetTimerManager().ClearTimer(PollTimerHandle);
    }

    World->GetTimerManager().SetTimer(PollTimerHandle, this, &ThisClass::HandlePollTick, PollIntervalSeconds, true);
    PollTimerWorld = World;
}

void UProximityVoiceLocalPlayerSubsystem::ResetVoiceStateAfterWorldTransition()
{
    bLocalTalkerRegistered = false;
    bNetworkedVoiceActive = false;
    bForceRemoteTalkerReregistration = true;
    bPendingInputDeviceApply = !PreferredInputDeviceId.IsEmpty();
    CachedLocalPingMs = INDEX_NONE;
    SetLocalMicActive(false);
}

void UProximityVoiceLocalPlayerSubsystem::EnsureLocalTalkerRegistration()
{
    if (bLocalTalkerRegistered)
    {
        return;
    }

    if (IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface())
    {
        bLocalTalkerRegistered = VoiceInterface->RegisterLocalTalker(GetLocalUserNum());
        UpdateNetworkedVoiceState();
    }
}

void UProximityVoiceLocalPlayerSubsystem::UpdateNetworkedVoiceState()
{
    if (IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface())
    {
        const uint8 LocalUserNum = GetLocalUserNum();
        const bool bShouldTransmit = !bPushToTalkMode || bPushToTalkPressed;

        if (bShouldTransmit && !bNetworkedVoiceActive)
        {
            VoiceInterface->StartNetworkedVoice(LocalUserNum);
            bNetworkedVoiceActive = true;
        }
        else if (!bShouldTransmit && bNetworkedVoiceActive)
        {
            VoiceInterface->StopNetworkedVoice(LocalUserNum);
            bNetworkedVoiceActive = false;
        }
    }
}

void UProximityVoiceLocalPlayerSubsystem::RefreshRemoteTalkers()
{
    UWorld* World = GetWorld();
    AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
    if (!GameState)
    {
        return;
    }

    APlayerController* LocalPlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
    APlayerState* LocalPlayerState = LocalPlayerController ? LocalPlayerController->PlayerState : nullptr;

    IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface();
    const bool bForceRemoteRegistration = bForceRemoteTalkerReregistration;

    bool bRosterChanged = false;
    TSet<FString> SeenPlayerIds;

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (!PlayerState || PlayerState == LocalPlayerState)
        {
            continue;
        }

        const FString RemotePlayerId = GetRemotePlayerId(PlayerState);
        if (RemotePlayerId.IsEmpty())
        {
            continue;
        }

        SeenPlayerIds.Add(RemotePlayerId);

        FRemoteTalkerRuntimeData& RuntimeData = RemoteTalkersById.FindOrAdd(RemotePlayerId);
        const APlayerState* PreviousPlayerState = RuntimeData.PlayerState.Get();
        const bool bPlayerStateChanged = PreviousPlayerState != PlayerState;
        RuntimeData.PlayerState = PlayerState;
        RuntimeData.UniqueNetId = GetRemoteUniqueNetId(PlayerState);

        UProximityVoiceTalkerComponent* Talker = RuntimeData.Talker.Get();
        if (bPlayerStateChanged && Talker)
        {
            Talker->DestroyComponent();
            RuntimeData.Talker.Reset();
            Talker = nullptr;
        }

        if (!Talker)
        {
            Talker = NewObject<UProximityVoiceTalkerComponent>(PlayerState);
            if (Talker)
            {
                Talker->RegisterComponent();
                Talker->RegisterWithPlayerState(PlayerState);
                Talker->SetGlobalVoiceVolume(GlobalRemoteVoiceVolume);
                Talker->SetRemotePreference(GetPreferenceFor(RemotePlayerId));
                ApplyVoiceSettingsToTalker(PlayerState, Talker);

                RuntimeData.Talker = Talker;
                bRosterChanged = true;
            }

            if (VoiceInterface.IsValid() && RuntimeData.UniqueNetId.IsValid())
            {
                VoiceInterface->RegisterRemoteTalker(*RuntimeData.UniqueNetId);
            }
        }
        else
        {
            if (bPlayerStateChanged || bForceRemoteRegistration)
            {
                Talker->RegisterWithPlayerState(PlayerState);
            }

            if (bForceRemoteRegistration && VoiceInterface.IsValid() && RuntimeData.UniqueNetId.IsValid())
            {
                VoiceInterface->RegisterRemoteTalker(*RuntimeData.UniqueNetId);
            }

            ApplyVoiceSettingsToTalker(PlayerState, Talker);
        }
    }

    TArray<FString> RemovedPlayerIds;
    for (const TPair<FString, FRemoteTalkerRuntimeData>& Pair : RemoteTalkersById)
    {
        if (!SeenPlayerIds.Contains(Pair.Key))
        {
            RemovedPlayerIds.Add(Pair.Key);
        }
    }

    for (const FString& RemovedPlayerId : RemovedPlayerIds)
    {
        if (FRemoteTalkerRuntimeData* RemovedData = RemoteTalkersById.Find(RemovedPlayerId))
        {
            if (VoiceInterface.IsValid() && RemovedData->UniqueNetId.IsValid())
            {
                VoiceInterface->UnregisterRemoteTalker(*RemovedData->UniqueNetId);
            }

            if (UProximityVoiceTalkerComponent* Talker = RemovedData->Talker.Get())
            {
                Talker->DestroyComponent();
            }
        }

        RemoteTalkersById.Remove(RemovedPlayerId);
        bRosterChanged = true;
    }

    if (bRosterChanged)
    {
        OnVoiceRosterChangedNative.Broadcast();
        OnVoiceRosterChangedDynamic.Broadcast();
    }

    if (bForceRemoteRegistration)
    {
        bForceRemoteTalkerReregistration = false;
    }
}

void UProximityVoiceLocalPlayerSubsystem::UpdateTalkingStateAndVolumes()
{
    const UProximityVoiceDeveloperSettings* VoiceSettings = GetDefault<UProximityVoiceDeveloperSettings>();
    const float MicThreshold = VoiceSettings ? VoiceSettings->MicActivityThreshold : 0.05f;
    const float FullVolumeDistance = VoiceSettings ? VoiceSettings->FullVolumeDistance : 250.0f;
    const float MaxVoiceDistance = VoiceSettings ? VoiceSettings->MaxVoiceDistance : 2500.0f;

    IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface();

    APlayerController* LocalPlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
    APawn* LocalPawn = LocalPlayerController ? LocalPlayerController->GetPawn() : nullptr;
    const FVector LocalPawnLocation = LocalPawn ? LocalPawn->GetActorLocation() : FVector::ZeroVector;
    bool bVoiceRosterViewChanged = false;

    for (TPair<FString, FRemoteTalkerRuntimeData>& Pair : RemoteTalkersById)
    {
        FRemoteTalkerRuntimeData& RuntimeData = Pair.Value;

        APlayerState* PlayerState = RuntimeData.PlayerState.Get();
        const int32 NewPingMs = ResolveRoundedPingMs(PlayerState);
        if (RuntimeData.LastPingMs != NewPingMs)
        {
            RuntimeData.LastPingMs = NewPingMs;
            bVoiceRosterViewChanged = true;
        }

        UProximityVoiceTalkerComponent* Talker = RuntimeData.Talker.Get();
        if (PlayerState && Talker)
        {
            APawn* RemotePawn = PlayerState->GetPawn();
            const FVector RemoteLocation = RemotePawn ? RemotePawn->GetActorLocation() : LocalPawnLocation;
            const float DistanceGain = LocalPawn && RemotePawn
                ? ComputeDistanceAttenuation(LocalPawnLocation, RemoteLocation, FullVolumeDistance, MaxVoiceDistance)
                : 1.0f;

            Talker->SetGlobalVoiceVolume(GlobalRemoteVoiceVolume);
            Talker->SetRemotePreference(GetPreferenceFor(Pair.Key));
            Talker->SetDistanceAttenuation(DistanceGain);
            ApplyVoiceSettingsToTalker(PlayerState, Talker);
        }

        float Amplitude = 0.0f;
        bool bIsTalking = false;

        if (VoiceInterface.IsValid() && RuntimeData.UniqueNetId.IsValid())
        {
            Amplitude = FMath::Max(0.0f, VoiceInterface->GetAmplitudeOfRemoteTalker(*RuntimeData.UniqueNetId));
            bIsTalking = VoiceInterface->IsRemotePlayerTalking(*RuntimeData.UniqueNetId) || Amplitude > (MicThreshold * 0.5f);
        }

        RuntimeData.LastAmplitude = Amplitude;
        if (RuntimeData.bWasTalking != bIsTalking)
        {
            RuntimeData.bWasTalking = bIsTalking;
            OnRemoteTalkingStateChangedNative.Broadcast(Pair.Key, bIsTalking);
            OnRemoteTalkingStateChangedDynamic.Broadcast(Pair.Key, bIsTalking);
        }
    }

    bool bLocalTalkingNow = false;
    if (VoiceInterface.IsValid())
    {
        bLocalTalkingNow = VoiceInterface->IsLocalPlayerTalking(GetLocalUserNum());
    }

    SetLocalMicActive(bLocalTalkingNow);

    const int32 NewLocalPingMs = ResolveRoundedPingMs(LocalPlayerController ? LocalPlayerController->PlayerState : nullptr);
    if (CachedLocalPingMs != NewLocalPingMs)
    {
        CachedLocalPingMs = NewLocalPingMs;
        bVoiceRosterViewChanged = true;
    }

    if (bVoiceRosterViewChanged)
    {
        OnVoiceRosterChangedNative.Broadcast();
        OnVoiceRosterChangedDynamic.Broadcast();
    }
}

void UProximityVoiceLocalPlayerSubsystem::ApplyVoiceSettingsToTalker(APlayerState* PlayerState, UProximityVoiceTalkerComponent* Talker) const
{
    if (!PlayerState || !Talker)
    {
        return;
    }

    const UProximityVoiceDeveloperSettings* VoiceSettings = GetDefault<UProximityVoiceDeveloperSettings>();
    if (!VoiceSettings)
    {
        return;
    }

    if (!CachedVoiceAttenuation.IsValid() && !VoiceSettings->VoiceAttenuation.IsNull())
    {
        CachedVoiceAttenuation = VoiceSettings->VoiceAttenuation.LoadSynchronous();
    }

    if (!CachedVoiceSourceEffects.IsValid() && !VoiceSettings->VoiceSourceEffects.IsNull())
    {
        CachedVoiceSourceEffects = VoiceSettings->VoiceSourceEffects.LoadSynchronous();
    }

    Talker->Settings.ComponentToAttachTo = ResolveAttachComponent(PlayerState);
    Talker->Settings.AttenuationSettings = CachedVoiceAttenuation.Get();
    Talker->Settings.SourceEffectChain = CachedVoiceSourceEffects.Get();
}

USceneComponent* UProximityVoiceLocalPlayerSubsystem::ResolveAttachComponent(const APlayerState* PlayerState) const
{
    const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
    if (!Pawn)
    {
        return nullptr;
    }

    if (USkeletalMeshComponent* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>())
    {
        return Mesh;
    }

    return Pawn->GetRootComponent();
}

APlayerState* UProximityVoiceLocalPlayerSubsystem::ResolveLocalPlayerState() const
{
    APlayerController* LocalPlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
    return LocalPlayerController ? LocalPlayerController->PlayerState : nullptr;
}

FString UProximityVoiceLocalPlayerSubsystem::GetRemotePlayerId(const APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return FString();
    }

    const FUniqueNetIdRepl& UniqueIdRepl = PlayerState->GetUniqueId();
    if (UniqueIdRepl.IsValid())
    {
        return UniqueIdRepl->ToString();
    }

    return FString::Printf(TEXT("PlayerState_%d"), PlayerState->GetPlayerId());
}

TSharedPtr<const FUniqueNetId> UProximityVoiceLocalPlayerSubsystem::GetRemoteUniqueNetId(const APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return nullptr;
    }

    const FUniqueNetIdRepl& UniqueIdRepl = PlayerState->GetUniqueId();
    return UniqueIdRepl.IsValid() ? UniqueIdRepl.GetUniqueNetId() : nullptr;
}

FProximityVoiceRemotePreference UProximityVoiceLocalPlayerSubsystem::GetPreferenceFor(const FString& RemotePlayerId) const
{
    if (const FProximityVoiceRemotePreference* FoundPreference = RemotePreferencesByPlayerId.Find(RemotePlayerId))
    {
        return *FoundPreference;
    }

    return FProximityVoiceRemotePreference{};
}

void UProximityVoiceLocalPlayerSubsystem::SavePreferences() const
{
    UProximityVoiceSaveGame* SaveGameObject = Cast<UProximityVoiceSaveGame>(UGameplayStatics::CreateSaveGameObject(UProximityVoiceSaveGame::StaticClass()));
    if (!SaveGameObject)
    {
        return;
    }

    SaveGameObject->RemotePreferences = RemotePreferencesByPlayerId;
    UGameplayStatics::SaveGameToSlot(SaveGameObject, GetSaveSlotName(), 0);
}

void UProximityVoiceLocalPlayerSubsystem::LoadPreferences()
{
    if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(GetSaveSlotName(), 0))
    {
        if (const UProximityVoiceSaveGame* LoadedVoiceSave = Cast<UProximityVoiceSaveGame>(LoadedGame))
        {
            RemotePreferencesByPlayerId = LoadedVoiceSave->RemotePreferences;
        }
    }
}

FString UProximityVoiceLocalPlayerSubsystem::GetSaveSlotName() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const int32 LocalUserNum = LocalPlayer ? LocalPlayer->GetControllerId() : 0;

    if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
    {
        if (IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
        {
            TSharedPtr<const FUniqueNetId> LocalNetId = Identity->GetUniquePlayerId(LocalUserNum);
            if (LocalNetId.IsValid())
            {
                return FString::Printf(TEXT("ProximityVoice_%s"), *LocalNetId->ToString());
            }
        }
    }

    return FString::Printf(TEXT("ProximityVoice_Local_%d"), LocalUserNum);
}

IOnlineVoicePtr UProximityVoiceLocalPlayerSubsystem::ResolveVoiceInterface() const
{
    if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
    {
        return OnlineSubsystem->GetVoiceInterface();
    }

    return nullptr;
}

uint8 UProximityVoiceLocalPlayerSubsystem::GetLocalUserNum() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const int32 LocalUserNum = LocalPlayer ? LocalPlayer->GetControllerId() : 0;
    return static_cast<uint8>(FMath::Max(0, LocalUserNum));
}

bool UProximityVoiceLocalPlayerSubsystem::ApplyPreferredInputDeviceBridge()
{
    if (!bPendingInputDeviceApply)
    {
        return true;
    }

    EnsureLocalTalkerRegistration();

    IOnlineVoicePtr VoiceInterface = ResolveVoiceInterface();
    if (!VoiceInterface.IsValid())
    {
        return false;
    }

    FVoiceEngineImpl* VoiceEngine = ResolveVoiceEngineImpl(VoiceInterface);
    TSharedPtr<IVoiceCapture> VoiceCapture = ResolveVoiceCapture(VoiceEngine);
    if (!VoiceCapture.IsValid())
    {
        return false;
    }

    FString RequestedDeviceName;
    if (!ResolveInputDeviceName(PreferredInputDeviceId, RequestedDeviceName))
    {
        RequestedDeviceName.Empty();
    }

    const bool bWasCapturing = VoiceCapture->IsCapturing();
    if (bWasCapturing)
    {
        VoiceCapture->Stop();
    }

    bool bApplied = VoiceCapture->ChangeDevice(RequestedDeviceName, UVOIPStatics::GetVoiceSampleRate(), UVOIPStatics::GetVoiceNumChannels());
    if (!bApplied && !RequestedDeviceName.IsEmpty())
    {
        bApplied = VoiceCapture->ChangeDevice(TEXT(""), UVOIPStatics::GetVoiceSampleRate(), UVOIPStatics::GetVoiceNumChannels());
    }

    if (bWasCapturing)
    {
        VoiceCapture->Start();
    }

    if (bApplied)
    {
        bPendingInputDeviceApply = false;
    }

    return bApplied;
}

bool UProximityVoiceLocalPlayerSubsystem::ResolveInputDeviceName(const FString& DeviceId, FString& OutDeviceName) const
{
    if (DeviceId.IsEmpty())
    {
        OutDeviceName.Empty();
        return true;
    }

    Audio::FAudioCapture AudioCapture;
    TArray<Audio::FCaptureDeviceInfo> Devices;
    AudioCapture.GetCaptureDevicesAvailable(Devices);

    const Audio::FCaptureDeviceInfo* MatchedDevice = Devices.FindByPredicate([&DeviceId](const Audio::FCaptureDeviceInfo& Device)
    {
        return Device.DeviceId.Equals(DeviceId, ESearchCase::CaseSensitive);
    });

    if (!MatchedDevice)
    {
        return false;
    }

    OutDeviceName = MatchedDevice->DeviceName;
    return true;
}

void UProximityVoiceLocalPlayerSubsystem::SetLocalMicActive(bool bInLocalMicActive)
{
    if (bLocalMicActive == bInLocalMicActive)
    {
        return;
    }

    bLocalMicActive = bInLocalMicActive;
    OnLocalMicActivityChangedNative.Broadcast(bLocalMicActive);
    OnLocalMicActivityChangedDynamic.Broadcast(bLocalMicActive);
}
