#include "Subsystems/AGASSInviteCodeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Settings/AGASSOnlineDeveloperSettings.h"
#include "Subsystems/AGASSSessionSubsystem.h"

FString UAGASSInviteCodeSubsystem::GenerateInviteCode() const
{
	static constexpr TCHAR Digits[] = TEXT("0123456789");

	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	const int32 InviteCodeLength = Settings ? Settings->InviteCodeLength : 6;

	FString InviteCode;
	InviteCode.Reserve(InviteCodeLength);

	for (int32 CodeIndex = 0; CodeIndex < InviteCodeLength; ++CodeIndex)
	{
		const int32 DigitIndex = FMath::RandRange(0, UE_ARRAY_COUNT(Digits) - 2);
		InviteCode.AppendChar(Digits[DigitIndex]);
	}

	return InviteCode;
}

FString UAGASSInviteCodeSubsystem::NormalizeInviteCode(const FString& InviteCode) const
{
	FString NormalizedInviteCode;
	NormalizedInviteCode.Reserve(InviteCode.Len());

	for (const TCHAR Character : InviteCode)
	{
		if (FChar::IsDigit(Character))
		{
			NormalizedInviteCode.AppendChar(Character);
		}
	}

	return NormalizedInviteCode;
}

void UAGASSInviteCodeSubsystem::JoinByInviteCode(const FString& InviteCode)
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSSessionSubsystem>() : nullptr)
	{
		SessionSubsystem->JoinInviteCode(InviteCode);
	}
}
