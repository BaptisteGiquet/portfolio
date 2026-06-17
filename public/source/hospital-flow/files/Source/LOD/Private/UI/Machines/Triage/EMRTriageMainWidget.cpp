

#include "UI/Machines/Triage/EMRTriageMainWidget.h"

#include "CommonTileView.h"
#include "Characters/Patient/EMRPatient.h"
#include "EngineUtils.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/EMRReceptionMachine.h"
#include "UI/Machines/Triage/EMRTriagePatientCardListEntry.h"
#include "UI/Machines/Triage/EMRTriagePatientCardObject.h"
#include "UI/Machines/Triage/EMRTriagePatientDetailWidget.h"
#include "UI/Patient/EMRPatientUIController.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/EMRTags.h"

namespace
{
	bool ShouldDisplayPatientInLegacyReceptionList(const AEMRPatient* Patient)
	{
		if (!Patient)
		{
			return false;
		}

		const UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
		if (!PatientASC)
		{
			return true;
		}

		return !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::WaitingExam)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::UnderExam)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::WaitingTreatment)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::UnderTreatment)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::WaitingToPay)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Paying)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Paid)
			&& !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving);
	}
}


void UEMRTriageMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CommonTileView_PatientCards)
	{
		CommonTileView_PatientCards->OnEntryWidgetReleased().AddUObject(this, &ThisClass::HandleEntryWidgetReleased);
		CommonTileView_PatientCards->OnItemClicked().RemoveAll(this);
		CommonTileView_PatientCards->OnItemClicked().AddUObject(this, &ThisClass::HandlePatientClicked);
	}

	RefreshPatientCards();

	if (AEMRReceptionMachine* ReceptionMachine = ResolveReceptionMachine())
	{
		if (UWidgetComponent* PanelComponent = ReceptionMachine->GetTriageDetailPanelWidgetComponent())
		{
			PanelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PanelComponent->SetHiddenInGame(true);
			PanelComponent->SetVisibility(false);
			CachedPanelWidgetComponent = PanelComponent;

			if (UEMRTriagePatientDetailWidget* PanelWidget = Cast<UEMRTriagePatientDetailWidget>(PanelComponent->GetUserWidgetObject()))
			{
				PanelWidget->ClearPatient();
				PanelWidget->SetVisibility(ESlateVisibility::Collapsed);
				PanelWidget->OnCloseRequested.RemoveAll(this);
				PanelWidget->OnCloseRequested.AddUObject(this, &ThisClass::HandlePanelCloseRequested);
				CachedPanelWidget = PanelWidget;
			}
		}
	}
}

void UEMRTriageMainWidget::NativeDestruct()
{
	ClearPatientCards();

	if (CommonTileView_PatientCards)
	{
		CommonTileView_PatientCards->OnEntryWidgetReleased().RemoveAll(this);
		CommonTileView_PatientCards->OnItemClicked().RemoveAll(this);
	}

	if (CachedPanelWidget.IsValid())
	{
		CachedPanelWidget->OnCloseRequested.RemoveAll(this);
	}

	if (WaitingRoomManager.IsValid())
	{
		WaitingRoomManager->OnSeatAssigned.RemoveDynamic(this, &ThisClass::OnSeatAssigned);
		WaitingRoomManager->OnSeatReleased.RemoveDynamic(this, &ThisClass::OnSeatReleased);
		WaitingRoomManager->OnPatientCalled.RemoveDynamic(this, &ThisClass::OnPatientCalled);
		WaitingRoomManager->OnWaitingPatientsChanged.RemoveDynamic(this, &ThisClass::HandleWaitingPatientsChanged);
	}

	WaitingRoomManager = nullptr;
	CachedPanelWidgetComponent = nullptr;
	CachedPanelWidget = nullptr;
	CachedReceptionMachine = nullptr;

	Super::NativeDestruct();
}

void UEMRTriageMainWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	ApplyFocusToPreferredWidget();

}

void UEMRTriageMainWidget::NativeOnDeactivated()
{
	OnTriageWidgetDeactivated.Broadcast();

	Super::NativeOnDeactivated();
}


UWidget* UEMRTriageMainWidget::GetPreferredFocusWidget_Implementation() const
{
	if (!CommonTileView_PatientCards)
	{
		return nullptr;
	}

	if (CommonTileView_PatientCards->GetNumItems() > 0)
	{
		if (UObject* Item = CommonTileView_PatientCards->GetItemAt(0))
		{
			if (UUserWidget* EntryWidget = CommonTileView_PatientCards->GetEntryWidgetFromItem(Item))
			{
				return EntryWidget;
			}
		}
	}

	return CommonTileView_PatientCards;
}

UWidget* UEMRTriageMainWidget::ResolvePreferredFocusWidget() const
{
	return GetPreferredFocusWidget_Implementation();
}

void UEMRTriageMainWidget::ApplyFocusToPreferredWidget()
{
	UWidget* PreferredWidget = ResolvePreferredFocusWidget();
	if (!PreferredWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	PreferredWidget->SetFocus();
}

void UEMRTriageMainWidget::RefreshPatientCards()
{
	ClearPatientCards();

	UEMRWaitingRoomManagerComponent* Manager = ResolveWaitingRoomManager();
	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommonTriage] No WaitingRoomManager found"));
		return;
	}

	if (!CommonTileView_PatientCards)
	{
		UE_LOG(LogTemp, Error, TEXT("[CommonTriage] Missing CommonTileView_PatientCards binding"));
		return;
	}

	TArray<FEMRWaitingPatientEntry> WaitingPatients = Manager->GetWaitingPatients();
	WaitingPatients.RemoveAllSwap([](const FEMRWaitingPatientEntry& Entry)
	{
		return !Entry.Patient.IsValid();
	});
	WaitingPatients.Sort([](const FEMRWaitingPatientEntry& Lhs, const FEMRWaitingPatientEntry& Rhs)
	{
		return Lhs.ArrivalTime < Rhs.ArrivalTime;
	});

	for (const FEMRWaitingPatientEntry& Entry : WaitingPatients)
	{
		AEMRPatient* Patient = Cast<AEMRPatient>(Entry.Patient.Get());
		if (!Patient)
		{
			continue;
		}

		if (!ShouldDisplayPatientInLegacyReceptionList(Patient))
		{
			continue;
		}

		UEMRPatientUIController* UIController = NewObject<UEMRPatientUIController>(this);
		if (!UIController)
		{
			continue;
		}
		
		UIController->SetRespectFolderDisplayRegistration(false);
		UIController->BindToPatient(Patient);
		UIController->OnPhaseChanged.RemoveDynamic(this, &ThisClass::HandlePatientPhaseChanged);
		UIController->OnPhaseChanged.AddDynamic(this, &ThisClass::HandlePatientPhaseChanged);

		UEMRTriagePatientCardObject* CardObject = NewObject<UEMRTriagePatientCardObject>(this);
		CardObject->Initialize(Patient, UIController);
		PatientCardObjects.Add(CardObject);
		CommonTileView_PatientCards->AddItem(CardObject);
	}
}

void UEMRTriageMainWidget::ClearPatientCards()
{
	for (UObject* ItemObject : PatientCardObjects)
	{
		if (UEMRTriagePatientCardObject* CardObject = Cast<UEMRTriagePatientCardObject>(ItemObject))
		{
			if (UEMRPatientUIController* UIController = CardObject->GetUIController())
			{
				UIController->OnPhaseChanged.RemoveDynamic(this, &ThisClass::HandlePatientPhaseChanged);
			}

			CardObject->Cleanup();
		}
	}

	PatientCardObjects.Empty();

	if (CommonTileView_PatientCards)
	{
		CommonTileView_PatientCards->ClearListItems();
	}
}

UEMRWaitingRoomManagerComponent* UEMRTriageMainWidget::ResolveWaitingRoomManager()
{
	if (WaitingRoomManager.IsValid())
	{
		return WaitingRoomManager.Get();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
	{
		if (AEMRWaitingRoomArea* WaitingArea = *It)
		{
			if (UEMRWaitingRoomManagerComponent* Manager = WaitingArea->GetManagerComponent())
			{
				WaitingRoomManager = Manager;
				Manager->OnSeatAssigned.AddDynamic(this, &ThisClass::OnSeatAssigned);
				Manager->OnSeatReleased.AddDynamic(this, &ThisClass::OnSeatReleased);
				Manager->OnPatientCalled.AddDynamic(this, &ThisClass::OnPatientCalled);
				Manager->OnWaitingPatientsChanged.AddDynamic(this, &ThisClass::HandleWaitingPatientsChanged);
				return Manager;
			}
		}
	}

	return nullptr;
}

void UEMRTriageMainWidget::OnSeatAssigned(UEMRWaitingSeatComponent* Seat, AActor* PatientActor)
{
	RefreshPatientCards();
}

void UEMRTriageMainWidget::OnSeatReleased(UEMRWaitingSeatComponent* Seat)
{
	RefreshPatientCards();
}

void UEMRTriageMainWidget::OnPatientCalled(AActor* PatientActor)
{
	RefreshPatientCards();

	if (PatientActor && CachedPanelWidget.IsValid())
	{
		if (CachedPanelWidget->GetActivePatient() == PatientActor)
		{
			HandlePanelCloseRequested();
		}
	}
}

void UEMRTriageMainWidget::HandleWaitingPatientsChanged()
{
	RefreshPatientCards();
}

void UEMRTriageMainWidget::HandlePatientPhaseChanged(FGameplayTag NewPhase)
{
	RefreshPatientCards();
}

void UEMRTriageMainWidget::HandleEntryWidgetReleased(UUserWidget& Widget)
{
	if (UEMRTriagePatientCardListEntry* EntryWidget = Cast<UEMRTriagePatientCardListEntry>(&Widget))
	{
		EntryWidget->ResetEntry();
	}
}

void UEMRTriageMainWidget::HandlePatientClicked(UObject* Item)
{
	UEMRTriagePatientCardObject* CardObject = Cast<UEMRTriagePatientCardObject>(Item);
	if (!CardObject || !CardObject->GetPatient())
	{
		return;
	}

	AEMRReceptionMachine* ReceptionMachine = ResolveReceptionMachine();
	if (!ReceptionMachine)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommonTriage] No reception machine found for triage detail panel."));
		return;
	}

	UWidgetComponent* PanelComponent = ReceptionMachine->GetTriageDetailPanelWidgetComponent();
	if (!PanelComponent)
	{
		return;
	}

	if (UEMRTriagePatientDetailWidget* PanelWidget = Cast<UEMRTriagePatientDetailWidget>(PanelComponent->GetUserWidgetObject()))
	{
		PanelWidget->SetPatient(CardObject->GetPatient());
		PanelWidget->SetVisibility(ESlateVisibility::Visible);
		PanelWidget->OnCloseRequested.RemoveAll(this);
		PanelWidget->OnCloseRequested.AddUObject(this, &ThisClass::HandlePanelCloseRequested);
		CachedPanelWidget = PanelWidget;
	}

	PanelComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PanelComponent->SetHiddenInGame(false);
	PanelComponent->SetVisibility(true);
	CachedPanelWidgetComponent = PanelComponent;
}

void UEMRTriageMainWidget::HandlePanelCloseRequested()
{
	if (CachedPanelWidget.IsValid())
	{
		CachedPanelWidget->ClearPatient();
		CachedPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CachedPanelWidgetComponent.IsValid())
	{
		CachedPanelWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CachedPanelWidgetComponent->SetHiddenInGame(true);
		CachedPanelWidgetComponent->SetVisibility(false);
	}
}

AEMRReceptionMachine* UEMRTriageMainWidget::ResolveReceptionMachine()
{
	if (CachedReceptionMachine.IsValid())
	{
		return CachedReceptionMachine.Get();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AEMRReceptionMachine> It(World); It; ++It)
	{
		if (AEMRReceptionMachine* ReceptionMachine = *It)
		{
			CachedReceptionMachine = ReceptionMachine;
			return ReceptionMachine;
		}
	}

	return nullptr;
}
