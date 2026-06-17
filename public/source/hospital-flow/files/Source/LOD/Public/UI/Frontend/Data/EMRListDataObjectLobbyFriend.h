#pragma once

#include "CoreMinimal.h"
#include "BlueprintDataDefinitions.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"
#include "EMRListDataObjectLobbyFriend.generated.h"

class UTexture2D;
/**
 * 
 */
UCLASS(BlueprintType)
class LOD_API UEMRListDataObjectLobbyFriend : public UEMRListDataObjectBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "EMR|Lobby")
	const FText& GetFriendName() const { return FriendName; }

	UFUNCTION(BlueprintPure, Category = "EMR|Lobby")
	const TSoftObjectPtr<UTexture2D>& GetFriendPortrait() const { return FriendPortrait; }

	UFUNCTION(BlueprintPure, Category = "EMR|Lobby")
	UTexture2D* GetFriendPortraitTexture() const { return FriendPortraitTexture; }

	UFUNCTION(BlueprintPure, Category = "EMR|Lobby")
	const FName& GetFriendId() const { return FriendId; }

	UFUNCTION(BlueprintPure, Category = "EMR|Lobby")
	const FBPUniqueNetId& GetFriendUniqueNetId() const { return FriendUniqueNetId; }

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendName(const FText& InFriendName);

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendPortrait(const TSoftObjectPtr<UTexture2D>& InFriendPortrait);

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendPortraitTexture(UTexture2D* InFriendPortraitTexture);

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendId(const FName& InFriendId);

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendUniqueNetId(const FBPUniqueNetId& InFriendUniqueNetId);

	UFUNCTION(BlueprintCallable, Category = "EMR|Lobby")
	void SetFriendData(const FName& InFriendId, const FText& InFriendName, const TSoftObjectPtr<UTexture2D>& InFriendPortrait);

protected:
	virtual void OnDataObjectInitialized() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Lobby", meta = (AllowPrivateAccess = "true"))
	FName FriendId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Lobby", meta = (AllowPrivateAccess = "true"))
	FText FriendName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Lobby", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> FriendPortrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Lobby", meta = (AllowPrivateAccess = "true"))
	FBPUniqueNetId FriendUniqueNetId;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FriendPortraitTexture;
};
