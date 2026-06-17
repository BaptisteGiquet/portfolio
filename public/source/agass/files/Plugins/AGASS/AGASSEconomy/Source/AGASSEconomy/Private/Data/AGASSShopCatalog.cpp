#include "Data/AGASSShopCatalog.h"

#include "Data/AGASSItemDefinition.h"

int32 FAGASSShopCatalogEntry::ResolvePurchaseCost(const UAGASSItemDefinition* ResolvedDefinition) const
{
	if (bOverridePurchaseCost)
	{
		return FMath::Max(PurchaseCostOverride, 0);
	}

	return ResolvedDefinition != nullptr ? FMath::Max(ResolvedDefinition->PurchaseCost, 0) : 0;
}

const FAGASSShopCatalogEntry* UAGASSShopCatalog::FindEntryById(const FName EntryId) const
{
	if (EntryId.IsNone())
	{
		return GetDefaultEntry();
	}

	for (const FAGASSShopCatalogEntry& Entry : Entries)
	{
		if (Entry.EntryId == EntryId)
		{
			return &Entry;
		}
	}

	return nullptr;
}

const FAGASSShopCatalogEntry* UAGASSShopCatalog::GetDefaultEntry() const
{
	return Entries.IsEmpty() ? nullptr : &Entries[0];
}
