#include "Lobby/EMRLobbyCharacterCatalog.h"


const FLobbyCharacterDefinition* UEMRLobbyCharacterCatalog::GetCharacterDefinition(int32 Index) const
{
    return Characters.IsValidIndex(Index) ? &Characters[Index] : nullptr;
}
