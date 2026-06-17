#include "UI/Machines/Triage/EMRTriagePatientCardListEntry.h"

#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Machines/Triage/EMRTriagePatientCardObject.h"

void UEMRTriagePatientCardListEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	CardObject = Cast<UEMRTriagePatientCardObject>(ListItemObject);
	if (!CardObject || !CardObject->GetPatient() || !CardObject->GetUIController())
	{
		ClearEntry();
		SetIsEnabled(false);
		return;
	}

	SetIsEnabled(true);

	if (Widget_TriagePatientCard)
	{
		Widget_TriagePatientCard->InitializeCard(CardObject->GetPatient(), CardObject->GetUIController());
	}
}

void UEMRTriagePatientCardListEntry::NativeDestruct()
{
	ClearEntry();
	Super::NativeDestruct();
}

void UEMRTriagePatientCardListEntry::ResetEntry()
{
	ClearEntry();
}

void UEMRTriagePatientCardListEntry::ClearEntry()
{
	if (Widget_TriagePatientCard)
	{
		Widget_TriagePatientCard->ResetCard();
	}

	CardObject = nullptr;
}
