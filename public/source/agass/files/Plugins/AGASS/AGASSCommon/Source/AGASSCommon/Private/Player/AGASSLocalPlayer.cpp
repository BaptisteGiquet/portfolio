#include "Player/AGASSLocalPlayer.h"

#include "Settings/AGASSSettingsLocal.h"

UAGASSSettingsLocal* UAGASSLocalPlayer::GetLocalSettings() const
{
	return UAGASSSettingsLocal::Get();
}
