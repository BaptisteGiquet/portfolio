#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AGASSFriendTimedRunEntryWidget.generated.h"

class UCommonTextBlock;

UCLASS()
class AGASSFRONTEND_API UAGASSFriendTimedRunEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitializeEntry(int32 InRank, const FString& InFriendName, int32 InTimeMilliseconds);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void RefreshView();

	int32 Rank = 0;
	FString FriendName;
	int32 TimeMilliseconds = 0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Label;
};
