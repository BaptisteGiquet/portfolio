

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "EMRLoadingScreenSubsystem.generated.h"

class IInputProcessor;

/**
 * 
 */
UCLASS()
class LOD_API UEMRLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
	
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingReasonUpdatedDelegate, const FText&, CurrentLoadingReason);
	
	UPROPERTY(BlueprintAssignable)
	FOnLoadingReasonUpdatedDelegate OnLoadingReasonUpdated;

	void RegisterLoadingProcessor(UObject* InLoadingProcessor);
	void UnregisterLoadingProcessor(UObject* InLoadingProcessor);

	UFUNCTION(BlueprintPure, Category = "EMR|Loading")
	FString GetDebugReasonForLoadingScreen() const { return DebugReasonForShowingOrHidingLoadingScreen; }
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	
	
private:
	void OnMapPreloaded(const FWorldContext& WorldContext, const FString& MapName);
	void OnMapPostLoaded(UWorld* LoadedWorld);
	
	void TryUpdateLoadingScreen();
	
	bool IsPreloadScreenActive() const;
	
	bool ShouldShowLoadingScreen();
	
	bool CheckTheNeedToShowLoadingScreen();
	
	void TryDisplayLoadingScreenIfNone();
	
	void TryRemoveLoadingScreen();
	
	void StartBlockingInput();
	void StopBlockingInput();
	void ChangePerformanceSettings(bool bEnableLoadingScreen);
	
	void NotifyLoadingScreenVisibilityChanged(bool bIsVisible);
	void LogLoadingScreenState(const TCHAR* ContextLabel, bool bShouldShowLoadingScreen);
	FString BuildLoadingScreenStateSnapshot() const;
	
	bool bIsCurrentlyLoadingMap = false;
	bool bIsLoadingVisible = false;
	bool bCanHoldLoadingScreenAfterNeed = false;
	float HoldLoadingScreenStartUpTime = -1.f;
	FString DebugReasonForShowingOrHidingLoadingScreen;
	FString LastNeedReasonForLoadingScreen = TEXT("(none)");
	FString LastLoggedDebugReason;
	double LastPeriodicStateLogTimeSeconds = 0.0;
	
	TSharedPtr<SWidget> CachedCreatedLoadingScreenWidget;
	TSharedPtr<IInputProcessor> InputPreProcessor;
	TArray<TWeakObjectPtr<UObject>> ExternalLoadingProcessors;
};
