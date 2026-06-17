#include "UI/Hub/EMRCommonHubNightShiftSelectionWidget.h"

#include "CommonTextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "TimerManager.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Core/EMRFrontendCommonListView.h"
#include "UI/Hub/EMRCommonHubNightShiftListDataObject.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Engine/EngineBaseTypes.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "UI/Frontend/Utils/DebugHelper.h"


void UEMRCommonHubNightShiftSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	const UWorld* World = GetWorld();
	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	const auto NetModeToString = [](ENetMode Mode)
	{
		switch (Mode)
		{
		case NM_Standalone: return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_Client: return TEXT("Client");
		default: return TEXT("Unknown");
		}
	};
	const FString NetModeStr = World ? NetModeToString(NetMode) : TEXT("NoWorld");
	const bool bIsServer = World && World->GetNetMode() < NM_Client;
	const bool bIsClient = World && World->GetNetMode() != NM_DedicatedServer;
    
	APlayerController* OwningPC = GetOwningPlayer();
	const int32 PlayerIndex = OwningPC ? UGameplayStatics::GetPlayerControllerID(OwningPC) : -1;
	const bool bIsLocalController = OwningPC ? OwningPC->IsLocalController() : false;

    CommonListView_NightShifts->OnItemIsHoveredChanged().AddUObject(this, &ThisClass::OnListViewItemHoveredChanged);
    CommonListView_NightShifts->OnItemSelectionChanged().AddUObject(this, &ThisClass::OnListViewItemSelectionChanged);
}


void UEMRCommonHubNightShiftSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CommonListView_NightShifts)
	{
		CommonListView_NightShifts->OnItemSelectionChanged().RemoveAll(this);
		CommonListView_NightShifts->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
	}

	if (CommonButton_Confirm)
	{
		CommonButton_Confirm->OnClicked().RemoveAll(this);
		CommonButton_Confirm->OnClicked().AddUObject(this, &ThisClass::OnConfirmClicked);
	}

	if (CommonButton_Cancel)
	{
		CommonButton_Cancel->OnClicked().RemoveAll(this);
		CommonButton_Cancel->OnClicked().AddUObject(this, &ThisClass::OnCancelClicked);
	}

	RefreshNightShifts();
	BindToGameStateUpdates();
}

void UEMRCommonHubNightShiftSelectionWidget::NativeDestruct()
{
	if (CommonListView_NightShifts)
	{
		CommonListView_NightShifts->OnItemSelectionChanged().RemoveAll(this);
	}

	if (CommonButton_Confirm)
	{
		CommonButton_Confirm->OnClicked().RemoveAll(this);
	}

	if (CommonButton_Cancel)
	{
		CommonButton_Cancel->OnClicked().RemoveAll(this);
	}

	UnbindFromGameStateUpdates();
	NightShiftEntries.Reset();

	Super::NativeDestruct();
}

void UEMRCommonHubNightShiftSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
}

void UEMRCommonHubNightShiftSelectionWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
}

UWidget* UEMRCommonHubNightShiftSelectionWidget::NativeGetDesiredFocusTarget() const
{
	if (GetPreferredFocusWidget_Implementation())
	{
		return GetPreferredFocusWidget_Implementation();
	}
	
	return Super::NativeGetDesiredFocusTarget();
}


UWidget* UEMRCommonHubNightShiftSelectionWidget::GetPreferredFocusWidget_Implementation() const
{
	if (!CommonListView_NightShifts)
	{
		return nullptr;
	}

	if (CommonListView_NightShifts->GetNumItems() > 0)
	{
		if (UObject* Item = CommonListView_NightShifts->GetItemAt(0))
		{
			if (UUserWidget* EntryWidget = CommonListView_NightShifts->GetEntryWidgetFromItem(Item))
			{
				return EntryWidget;
			}
		}
	}

	return CommonListView_NightShifts;
}


void UEMRCommonHubNightShiftSelectionWidget::RefreshNightShifts()
{
	if (!CommonListView_NightShifts)
	{
		return;
	}

	CommonListView_NightShifts->ClearListItems();
	NightShiftEntries.Reset();

	bool bHasProgression = false;
	TArray<UEMRCommonHubNightShiftListDataObject*> Entries;

	BuildEntriesFromGameState(Entries, bHasProgression);

	if (Entries.Num() == 0)
	{
		BuildEntriesFromSubsystem(Entries, bHasProgression);
	}

	for (UEMRCommonHubNightShiftListDataObject* Entry : Entries)
	{
		NightShiftEntries.Add(Entry);
		CommonListView_NightShifts->AddItem(Entry);
	}

	SelectedNightShift = nullptr;
	SelectedOfferIndex = INDEX_NONE;
	UpdateConfirmState(false);
}


void UEMRCommonHubNightShiftSelectionWidget::BuildEntriesFromSubsystem(TArray<UEMRCommonHubNightShiftListDataObject*>& OutEntries, bool& bOutHasProgression)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UEMRRunProgressionSubsystem* ProgressionSubsystem = World->GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>();
	if (!ProgressionSubsystem)
	{
		return;
	}

	bOutHasProgression = ProgressionSubsystem->HasCachedState();

	TArray<UEMRNightShiftDefinition*> CachedNightShifts;
	ProgressionSubsystem->GetCachedNextNightShifts(CachedNightShifts);

	for (int32 Index = 0; Index < CachedNightShifts.Num(); ++Index)
	{
		UEMRNightShiftDefinition* Definition = CachedNightShifts[Index];
		if (!Definition)
		{
			continue;
		}

		UEMRCommonHubNightShiftListDataObject* EntryObject = NewObject<UEMRCommonHubNightShiftListDataObject>(this);
		EntryObject->Init(Definition, bOutHasProgression, Index);
		OutEntries.Add(EntryObject);
	}
}


void UEMRCommonHubNightShiftSelectionWidget::BuildEntriesFromGameState(TArray<UEMRCommonHubNightShiftListDataObject*>& OutEntries, bool bSubsystemHasProgression)
{
	const UWorld* World = GetWorld();
	const AEMRNightShiftGameState* RunGS = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;

	if (!RunGS)
	{
		return;
	}

	if (const UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (const UEMRRunProgressionSubsystem* ProgressionSubsystem = GameInstance->GetSubsystem<UEMRRunProgressionSubsystem>())
		{
			bSubsystemHasProgression = ProgressionSubsystem->HasCachedState();
		}
	}

	const TArray<UEMRNightShiftDefinition*>& AvailableNightShifts = RunGS->GetAvailableNextNightShifts();

	for (int32 Index = 0; Index < AvailableNightShifts.Num(); ++Index)
	{
		UEMRNightShiftDefinition* Definition = AvailableNightShifts[Index];
		if (!Definition)
		{
			continue;
		}

		const bool bUnlocked = bSubsystemHasProgression || RunGS->GetRunPhase() == EER_RunPhase::Hub;

		UEMRCommonHubNightShiftListDataObject* EntryObject = NewObject<UEMRCommonHubNightShiftListDataObject>(this);
		EntryObject->Init(Definition, bUnlocked, Index);
		OutEntries.Add(EntryObject);
	}
}


void UEMRCommonHubNightShiftSelectionWidget::HandleSelectionChanged(UObject* SelectedItem)
{
    UpdateSelectionFromItem(SelectedItem);
}


void UEMRCommonHubNightShiftSelectionWidget::UpdateSelectionFromItem(UObject* SelectedItem)
{
    UEMRCommonHubNightShiftListDataObject* EntryObject = Cast<UEMRCommonHubNightShiftListDataObject>(SelectedItem);
    const bool bIsUnlockedSelection = EntryObject && EntryObject->IsUnlocked();

	SelectedNightShift = bIsUnlockedSelection ? EntryObject->GetDefinition() : nullptr;
	SelectedOfferIndex = bIsUnlockedSelection ? EntryObject->GetOfferIndex() : INDEX_NONE;

	UpdateConfirmState(bIsUnlockedSelection);
}


void UEMRCommonHubNightShiftSelectionWidget::UpdateConfirmState(bool bHasUnlockedSelection)
{
	const bool bCanConfirm = bHasUnlockedSelection && SelectedNightShift != nullptr && !bSelectionCommitted;

	if (CommonButton_Confirm)
	{
		CommonButton_Confirm->SetIsEnabled(bCanConfirm);
	}

	BP_OnConfirmAvailabilityChanged(bCanConfirm, bSelectionCommitted);
}


void UEMRCommonHubNightShiftSelectionWidget::OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered)
{
	if (!InHoveredItem) return;
	
	UEMRFrontendCommonListEntryWidgetBase* HoveredEntryWidget = CommonListView_NightShifts->GetEntryWidgetFromItem<UEMRFrontendCommonListEntryWidgetBase>(InHoveredItem);
	check(HoveredEntryWidget);
	
	HoveredEntryWidget->NativeOnListEntryWidgetHovered(bWasHovered);
}


void UEMRCommonHubNightShiftSelectionWidget::OnListViewItemSelectionChanged(UObject* InSelectedItem)
{
	
}


void UEMRCommonHubNightShiftSelectionWidget::OnConfirmClicked()
{
	if (!SelectedNightShift || SelectedOfferIndex == INDEX_NONE || bSelectionCommitted)
	{
		return;
	}

	if (AEMRPlayerController* OwningPC = GetOwningPlayer<AEMRPlayerController>())
	{
		OwningPC->Server_SelectNightShiftByIndex(SelectedOfferIndex);
	}
}


void UEMRCommonHubNightShiftSelectionWidget::OnCancelClicked()
{
    SelectedNightShift = nullptr;
    SelectedOfferIndex = INDEX_NONE;

	if (CommonListView_NightShifts)
	{
		CommonListView_NightShifts->ClearSelection();
	}

	UpdateConfirmState(false);
}


void UEMRCommonHubNightShiftSelectionWidget::BindToGameStateUpdates()
{
	UnbindFromGameStateUpdates();

	AEMRNightShiftGameState* RunGS = ResolveNightShiftGameState();
	if (!RunGS)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(GameStateBindingRetryHandle, this, &ThisClass::BindToGameStateUpdates, 0.2f, false);
		}
		return;
	}

	CachedNightShiftGameState = RunGS;
	GameStateDelegateHandle = RunGS->OnNextNightShiftsAvailableChanged().AddUObject(this, &ThisClass::OnGameStateNightShiftsUpdated);
	RunGS->OnNightShiftSelectionCommittedChanged().AddUObject(this, &ThisClass::OnGameStateSelectionCommittedChanged);

	OnGameStateSelectionCommittedChanged();
}


void UEMRCommonHubNightShiftSelectionWidget::UnbindFromGameStateUpdates()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameStateBindingRetryHandle);
	}

	if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
	{
		if (GameStateDelegateHandle.IsValid())
		{
			RunGS->OnNextNightShiftsAvailableChanged().Remove(GameStateDelegateHandle);
		}

		RunGS->OnNightShiftSelectionCommittedChanged().RemoveAll(this);
	}

	GameStateDelegateHandle.Reset();
	CachedNightShiftGameState.Reset();
}


AEMRNightShiftGameState* UEMRCommonHubNightShiftSelectionWidget::ResolveNightShiftGameState() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
}


void UEMRCommonHubNightShiftSelectionWidget::OnGameStateNightShiftsUpdated()
{
	RefreshNightShifts();
}


void UEMRCommonHubNightShiftSelectionWidget::OnGameStateSelectionCommittedChanged()
{
    bSelectionCommitted = CachedNightShiftGameState.IsValid() ? CachedNightShiftGameState->IsNightShiftSelectionCommitted() : false;
    UpdateConfirmState(SelectedNightShift != nullptr);
}
