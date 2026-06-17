
#pragma once

#include "NativeGameplayTags.h"

namespace MobaObjectTypeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Object_Actor_PlayerController);
}


namespace MobaCharacterTypeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Hero);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Minion);
}


namespace MobaStatusTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_Full);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_Empty);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Mana_Full);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Mana_Empty);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stun);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Aiming);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Crosshair);
}


namespace MobaGameplayAbilityPressedTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_BasicAttack_Pressed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aim_Pressed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AbilityOne_Pressed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AbilityTwo_Pressed);
}


namespace MobaGameplayAbilityReleasedTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_BasicAttack_Released);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aim_Released);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AbilityOne_Released);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_AbilityTwo_Released);
}


namespace MobaAbilityCostTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cost);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cost_Mana);
}



namespace MobaGameplayCueTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_CameraShake);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Crunch_Punch);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Crunch_GroundBlast);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Phase_Shoot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Phase_BlackHole);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Minion);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Reaction);
}



namespace MobaPushEventTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_Uppercut_Launch);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_Uppercut_Final);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_Uppercut_Hold);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_GroundBlast);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_Laser);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_Push_BlackHole);
}


namespace MobaAbilityAimingEventTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aiming);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aiming_Crunch_GroundBlast);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aiming_Phase_Shoot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aiming_Phase_Laser);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Aiming_Phase_BlackHole);
}

namespace MobaTargetTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_Updated);
}

namespace MobaDataTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Attribute);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Attribute_Experience);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Attribute_Gold);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Cost);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_Mana);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_MaxMana);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_Strength);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_Intelligence);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_MagicResistance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_AttackDamage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_AbilityPower);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_AttackSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_ArmorPenetration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_MagicResistancePenetration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_MoveSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shop_Item_Effect_CooldownReduction);
}


/*********************************************************/
/************************CRUNCH***************************/
/*********************************************************/

namespace MobaAbilityCrunchComboChangeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change_Phase1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change_Phase2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change_Phase3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change_Phase4);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Change_End);
}


namespace MobaAbilityCrunchUppercutChangeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Change);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Change_Phase2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Change_Phase3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Change_End);
}


namespace MobaCrunchDamageEventTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Combo_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Damage_Launch);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Damage_Phase2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Damage_Phase3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_GroundBlast_Damage);
}


namespace MobaCrunchCooldownEventTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_Uppercut_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Crunch_GroundBlast_Cooldown);
}


/*********************************************************/
/************************Phase***************************/
/*********************************************************/


namespace MobaPhaseShootTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Shoot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Shoot_HandL);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Shoot_HandR);
}

namespace MobaPhaseLaserTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Laser);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Laser_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Laser_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Laser_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_Laser_Cost);
}

namespace MobaPhaseBlackHoleTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_BlackHole);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_BlackHole_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_BlackHole_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Phase_BlackHole_Cost);
}
