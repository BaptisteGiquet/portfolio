#include "UI/AGASSFrontendPrototypeButtonBase.h"

#include "CommonTextBlock.h"

void UAGASSFrontendPrototypeButtonBase::SetButtonText(const FText& InText)
{
	if (Text_Label != nullptr)
	{
		Text_Label->SetText(InText);
	}
}
