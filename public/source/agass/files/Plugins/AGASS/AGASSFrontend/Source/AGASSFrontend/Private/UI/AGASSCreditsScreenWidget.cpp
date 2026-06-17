#include "UI/AGASSCreditsScreenWidget.h"

UAGASSCreditsScreenWidget::UAGASSCreditsScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "FrontendBackActionName", "Back");
}

FText UAGASSCreditsScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "CreditsTitle", "Credits");
}

void UAGASSCreditsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UAGASSCreditsScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsIntro", "This dedicated credits screen is part of the persistent AGASS frontend flow. Replace the placeholder text with authored credits content, logos, links, and legal attribution in the final Widget Blueprint pass."), 16);
	AddSpacer(ContentBox, 8.f);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsCoreHeading", "Core Project"), 18);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsCoreBody", "AGASS core game, frontend shell, online flow, and mod architecture."), 16);
	AddSpacer(ContentBox, 8.f);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsModsHeading", "Mods And Community"), 18);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsModsBody", "Installed maps and mod packs can contribute their own credit sections later through data-driven frontend extensions without replacing the root shell."), 16);
	AddSpacer(ContentBox, 8.f);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsAudioHeading", "Audio And Content"), 18);
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsAudioBody", "Sound libraries, music, external content, and marketplace attributions belong here once the final authored list is assembled."), 16);
}

UWidget* UAGASSCreditsScreenWidget::ResolveInitialFocusTarget() const
{
	return nullptr;
}
