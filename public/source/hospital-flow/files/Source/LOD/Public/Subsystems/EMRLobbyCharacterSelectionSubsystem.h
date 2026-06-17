#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "EMRLobbyCharacterSelectionSubsystem.generated.h"

UCLASS()
class LOD_API UEMRLobbyCharacterSelectionSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    int32 GetCurrentSelectionIndex() const { return CurrentSelectionIndex; }
    void SetSelectionIndex(int32 NewIndex);
    void ApplySavedSelectionToPlayerState();

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyCharacterSelectionChanged, int32);
    FOnLobbyCharacterSelectionChanged OnSelectionChanged;

private:
    int32 GetFirstCatalogCharacterIndex() const;
    FString GetSaveSlotName() const;
    void LoadSelection();
    void SaveSelection() const;
    void ApplySelectionToPlayerState();
    int32 ResolveSelectionIndex(int32 InIndex) const;

    int32 CurrentSelectionIndex = INDEX_NONE;
    FTimerHandle ApplySelectionRetryHandle;
};
