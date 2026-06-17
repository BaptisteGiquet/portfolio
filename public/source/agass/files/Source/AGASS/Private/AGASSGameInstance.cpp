#include "AGASSGameInstance.h"

#include "Misc/Parse.h"
#include "Subsystems/AGASSSessionSubsystem.h"

bool UAGASSGameInstance::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	FString InviteCodeParameter;
	if (FParse::Value(Cmd, TEXT("AGASSJOINCODE="), InviteCodeParameter))
	{
		if (UAGASSSessionSubsystem* SessionSubsystem = GetSubsystem<UAGASSSessionSubsystem>())
		{
			SessionSubsystem->JoinInviteCode(InviteCodeParameter);
			Ar.Logf(TEXT("AGASSJOINCODE submitted with invite code '%s'."), *InviteCodeParameter);
			return true;
		}
	}

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSubsystem<UAGASSSessionSubsystem>())
	{
		if (FParse::Command(&Cmd, TEXT("AGASSHOST")))
		{
			SessionSubsystem->HostSession();
			Ar.Log(TEXT("AGASSHOST submitted."));
			return true;
		}

		if (FParse::Command(&Cmd, TEXT("AGASSFINDSESSIONS")))
		{
			SessionSubsystem->FindSessions();
			Ar.Log(TEXT("AGASSFINDSESSIONS submitted."));
			return true;
		}

		if (FParse::Command(&Cmd, TEXT("AGASSJOINCODE")))
		{
			FString InviteCode = FParse::Token(Cmd, false);
			if (InviteCode.IsEmpty())
			{
				InviteCode = FString(Cmd).TrimStartAndEnd();
			}
			SessionSubsystem->JoinInviteCode(InviteCode);
			Ar.Logf(TEXT("AGASSJOINCODE submitted with invite code '%s'."), *InviteCode);
			return true;
		}

		if (FParse::Command(&Cmd, TEXT("AGASSRETURNTOFRONTEND")))
		{
			SessionSubsystem->ReturnToFrontend(TEXT("Requested via AGASSRETURNTOFRONTEND."), true);
			Ar.Log(TEXT("AGASSRETURNTOFRONTEND submitted."));
			return true;
		}
	}

	return Super::Exec(InWorld, Cmd, Ar);
}
