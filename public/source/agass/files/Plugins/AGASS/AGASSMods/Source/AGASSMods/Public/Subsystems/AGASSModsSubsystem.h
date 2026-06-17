#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Types/AGASSModsTypes.h"
#include "AGASSModsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FAGASSModsRegistryChangedEvent);
DECLARE_MULTICAST_DELEGATE(FAGASSModsSelectionChangedEvent);

UCLASS()
class AGASSMODS_API UAGASSModsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RefreshRegistry();

	const TArray<FAGASSDiscoveredModInfo>& GetDiscoveredMods() const;
	const TArray<FAGASSAvailableMapInfo>& GetAvailableMaps() const;
	const TArray<FString>& GetActiveModIds() const;
	FString GetSelectedMapId() const;
	bool IsModActive(const FString& ModId) const;

	const FAGASSAvailableMapInfo* GetSelectedMapInfo() const;
	const FAGASSAvailableMapInfo* FindMapInfoByWorldPackagePath(const FString& WorldPackagePath) const;

	bool SetModActive(const FString& ModId, bool bActive, FString& OutFailureMessage);
	bool SetSelectedMapId(const FString& MapId, FString& OutFailureMessage);

	bool TryBuildHostedSelection(FAGASSResolvedContentSelection& OutSelection, FString& OutFailureMessage) const;
	bool TryBuildRuntimeSelectionForWorld(const UWorld& World, FAGASSResolvedContentSelection& OutSelection, FString& OutFailureMessage) const;
	bool EvaluateJoinCompatibility(
		const FString& RemoteMapId,
		const FString& RemoteMapDisplayName,
		const FString& RemoteActiveModIdsCsv,
		const FString& RemoteContentCompatibilityHash,
		FAGASSJoinCompatibilityResult& OutResult) const;
	bool IsGameplaySessionWorld(const UWorld& World) const;

	FAGASSModsRegistryChangedEvent& OnRegistryChanged();
	FAGASSModsSelectionChangedEvent& OnSelectionChanged();

private:
	bool CanChangeSelection(FString& OutFailureMessage) const;
	void RegisterConfiguredPluginSearchPaths();
	void RebuildRegistry();
	void AddConfiguredBaseMaps();
	void RefreshDynamicFlags();
	void ValidateSelectionState();

	bool TryResolveSelection(
		const FString& MapId,
		const TArray<FString>& RequestedActiveModIds,
		FAGASSResolvedContentSelection& OutSelection,
		FString& OutFailureMessage) const;

	bool TryResolveActiveMods(
		const TArray<FString>& RequestedActiveModIds,
		TArray<const FAGASSDiscoveredModInfo*>& OutResolvedMods,
		FString& OutFailureMessage) const;

	FString BuildContentCompatibilityHash(
		const FAGASSAvailableMapInfo& MapInfo,
		const TArray<const FAGASSDiscoveredModInfo*>& ActiveMods) const;

	static void NormalizeStringArray(TArray<FString>& InOutValues);
	static TArray<FString> ParseDelimitedIds(const FString& DelimitedValue);
	static FString JoinIds(const TArray<FString>& Values);

	TArray<FAGASSDiscoveredModInfo> DiscoveredMods;
	TArray<FAGASSAvailableMapInfo> AvailableMaps;
	TMap<FString, int32> ModIndexById;
	TMap<FString, int32> MapIndexById;
	TMap<FString, int32> MapIndexByWorldPackagePath;
	TSet<FString> AddedPluginSearchPaths;
	TSet<FString> MountedPluginPakFiles;
	TArray<FString> ActiveModIds;
	FString SelectedMapId;

	FAGASSModsRegistryChangedEvent RegistryChangedEvent;
	FAGASSModsSelectionChangedEvent SelectionChangedEvent;
};
