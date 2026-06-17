#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AGASSInviteCodeSubsystem.generated.h"

UCLASS()
class AGASSONLINE_API UAGASSInviteCodeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FString GenerateInviteCode() const;
	FString NormalizeInviteCode(const FString& InviteCode) const;
	void JoinByInviteCode(const FString& InviteCode);
};
