
#include "GAS/MobaGameplayTags.h"

namespace MobaObjectTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Object_Actor_PlayerController, "Object.Actor.PlayerController", "This actor is a PlayerController");
}


namespace MobaCharacterTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Hero, "Character.Hero", "This Character is a Hero");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Minion, "Character.Minion", "This Character is a Minion");
}


namespace MobaStatusTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Health_Full, "Status.Health.Full", "This actor is Full Health");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Health_Empty, "Status.Health.Empty", "This actor Health is Empty");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Mana_Full, "Status.Mana.Full", "This actor is Full Mana");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Mana_Empty, "Status.Mana.Empty", "This actor Mana is Empty");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Dead, "Status.Dead", "This actor is Dead");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Stun, "Status.Stun", "This actor is Stun");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Aiming, "Status.Aiming", "This actor is Aiming");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Crosshair, "Status.Crosshair", "Crosshair");
}


namespace MobaGameplayAbilityPressedTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_BasicAttack_Pressed, "Ability.BasicAttack.Pressed", "Basic attack pressed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aim_Pressed, "Ability.Aim.Pressed", "Aim pressed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_AbilityOne_Pressed, "Ability.AbilityOne.Pressed", "Ability One pressed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_AbilityTwo_Pressed, "Ability.AbilityTwo.Pressed", "Ability Two pressed");
}


namespace MobaGameplayAbilityReleasedTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_BasicAttack_Released, "Ability.BasicAttack.Released", "Basic attack released");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aim_Released, "Ability.Aim.Released", "Aim released");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_AbilityOne_Released, "Ability.AbilityOne.Released", "Ability One released");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_AbilityTwo_Released, "Ability.AbilityTwo.Released", "Ability Two released");
}



namespace MobaAbilityCostTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cost, "Ability.Cost", "Ability cost");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cost_Mana, "Ability.Cost.Mana", "Ability Mana cost");
}


namespace MobaGameplayCueTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake, "GameplayCue.CameraShake", "Camera Shake");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Reaction, "GameplayCue.Hit.Reaction", "GameplayCue.Hit.Reaction");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Crunch_Punch, "GameplayCue.Hit.Crunch.Punch", "GameplayCue.Hit.Crunch.Punch");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Crunch_GroundBlast, "GameplayCue.Hit.Crunch.GroundBlast", "GameplayCue.Hit.Crunch.GroundBlast");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Phase_Shoot, "GameplayCue.Hit.Phase.Shoot", "GameplayCue.Hit.Phase.Shoot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Phase_BlackHole, "GameplayCue.Hit.Phase.BlackHole", "GameplayCue.Hit.Phase.BlackHole");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Minion, "GameplayCue.Hit.Minion", "GameplayCue.Hit.Minion");
	
}


namespace MobaPushEventTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push, "Ability.Passive.Push", "To Get all children of Ability.Passive.Push")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_Uppercut_Launch, "Ability.Passive.Push.Uppercut.Launch", "To Get all children of Ability.Passive.Push.Uppercut.Launch")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_Uppercut_Final, "Ability.Passive.Push.Uppercut.Final", "To Get all children of Ability.Passive.Push.Uppercut.Final")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_Uppercut_Hold, "Ability.Passive.Push.Uppercut.Hold", "To Get all children of Ability.Passive.Push.Uppercut.Hold")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_GroundBlast, "Ability.Passive.Push.GroundBlast", "To Get all children of Ability.Passive.Push.GroundBlast")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_Laser, "Ability.Passive.Push.Laser", "To Get all children of Ability.Passive.Push.Laser")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Passive_Push_BlackHole, "Ability.Passive.Push.BlackHole", "To Get all children of Ability.Passive.Push.BlackHole")
}


namespace MobaAbilityAimingEventTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aiming, "Ability.Aiming", "To Get all children of Ability.Aiming")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aiming_Crunch_GroundBlast, "Ability.Aiming.Crunch.GroundBlast", "To Get all children of Ability.Aiming.Crunch.GroundBlast")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aiming_Phase_Shoot, "Ability.Aiming.Phase.Shoot", "To Get all children of Ability.Aiming.Phase.Shoot")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aiming_Phase_Laser, "Ability.Aiming.Phase.Laser", "To Get all children of Ability.Aiming.Phase.Laser")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Aiming_Phase_BlackHole, "Ability.Aiming.Phase.BlackHole", "To Get all children of Ability.Aiming.Phase.BlackHole")
}

namespace MobaTargetTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target, "Target", "To Get all children of Target")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Target_Updated, "Target.Updated", "To Get all children of Target.Updated")
}

namespace MobaDataTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Attribute, "Data.Attribute", "To Get all children of Data.Attribute")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Attribute_Experience, "Data.Attribute.Experience", "To Get Experience Attribute")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Attribute_Gold, "Data.Attribute.Gold", "To Get Gold Attribute")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Shop, "Data.Shop", "To Get Shop Data")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Shop_Item, "Data.Shop.Item", "To Get Shop Item Data")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Shop_Item_Cost, "Data.Shop.Item.Cost", "To Get Shop Item Cost Data")

	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect, TEXT("Data.Shop.Item.Effect"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_Health,                       TEXT("Data.Shop.Item.Effect.Health"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_MaxHealth,                    TEXT("Data.Shop.Item.Effect.MaxHealth"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_Mana,                       TEXT("Data.Shop.Item.Effect.Mana"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_MaxMana,                      TEXT("Data.Shop.Item.Effect.MaxMana"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_Strength,                     TEXT("Data.Shop.Item.Effect.Strength"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_Intelligence,                 TEXT("Data.Shop.Item.Effect.Intelligence"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_Armor,                        TEXT("Data.Shop.Item.Effect.Armor"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_MagicResistance,              TEXT("Data.Shop.Item.Effect.MagicResistance"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_AttackDamage,                 TEXT("Data.Shop.Item.Effect.AttackDamage"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_AbilityPower,                 TEXT("Data.Shop.Item.Effect.AbilityPower"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_AttackSpeed,                  TEXT("Data.Shop.Item.Effect.AttackSpeed"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_ArmorPenetration,             TEXT("Data.Shop.Item.Effect.ArmorPenetration"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_MagicResistancePenetration,   TEXT("Data.Shop.Item.Effect.MagicResistancePenetration"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_MoveSpeed,                    TEXT("Data.Shop.Item.Effect.MoveSpeed"));
	UE_DEFINE_GAMEPLAY_TAG(MobaDataTags::Data_Shop_Item_Effect_CooldownReduction,            TEXT("Data.Shop.Item.Effect.CooldownReduction"));
}

/*********************************************************/
/************************CRUNCH***************************/
/*********************************************************/


namespace MobaAbilityCrunchComboChangeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change, "Ability.Crunch.Combo.Change", "To Get all childs of Ability.Crunch.Combo.Change")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change_Phase1, "Ability.Crunch.Combo.Change.Phase1", "Gameplay Event Notify to go to Phase1")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change_Phase2, "Ability.Crunch.Combo.Change.Phase2", "Gameplay Event Notify to go to Phase2")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change_Phase3, "Ability.Crunch.Combo.Change.Phase3", "Gameplay Event Notify to go to Phase3")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change_Phase4, "Ability.Crunch.Combo.Change.Phase4", "Gameplay Event Notify to go to Phase4")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Change_End, "Ability.Crunch.Combo.Change.End", "Gameplay Event Notify to go to End")
}


namespace MobaAbilityCrunchUppercutChangeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Change, "Ability.Crunch.Uppercut.Change", "To Get all children of Ability.Crunch.Uppercut.Change")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Change_Phase2, "Ability.Crunch.Uppercut.Change.Phase2", "Gameplay Event Notify to go to Phase2")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Change_Phase3, "Ability.Crunch.Uppercut.Change.Phase3", "Gameplay Event Notify to go to Phase3")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Change_End, "Ability.Crunch.Uppercut.Change.End", "Gameplay Event Notify to go to End")
}


namespace MobaCrunchDamageEventTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Combo_Damage, "Ability.Crunch.Combo.Damage", "To Get all children of Ability.Crunch.Combo.Damage")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Damage, "Ability.Crunch.Uppercut.Damage", "To Get all children of Ability.Crunch.Uppercut.Damage")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Damage_Launch, "Ability.Crunch.Uppercut.Damage.Launch", "To Get all children of Ability.Uppercut.Damage.Launch")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Damage_Phase2, "Ability.Crunch.Uppercut.Damage.Phase2", "To Get all children of Ability.Uppercut.Damage.Phase2")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Damage_Phase3, "Ability.Crunch.Uppercut.Damage.Phase3", "To Get all children of Ability.Uppercut.Damage.Phase3")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_GroundBlast_Damage, "Ability.Crunch.GroundBlast.Damage", "To Get all children of Ability.Crunch.GroundBlast.Damage")
}

namespace MobaCrunchCooldownEventTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_Uppercut_Cooldown, "Ability.Crunch.Uppercut.Cooldown", "To Get all children of Ability.Crunch.Uppercut.Cooldown")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Crunch_GroundBlast_Cooldown, "Ability.Crunch.GroundBlast.Cooldown", "To Get all children of Ability.Crunch.GroundBlast.Cooldown")
}


/*********************************************************/
/************************Phase***************************/
/*********************************************************/


namespace MobaPhaseShootTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Shoot, "Ability.Phase.Shoot", "To get Phase Shoot Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Shoot_HandL, "Ability.Phase.Shoot.HandL", "To get Phase Shoot Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Shoot_HandR, "Ability.Phase.Shoot.HandR", "To get Phase Shoot Ability")
}

namespace MobaPhaseLaserTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Laser, "Ability.Phase.Laser", "To get Phase Laser Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Laser_Fire, "Ability.Phase.Laser.Fire", "To get Phase Laser Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Laser_Damage, "Ability.Phase.Laser.Damage", "To get Phase Laser Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Laser_Cooldown, "Ability.Phase.Laser.Cooldown", "To get Phase Laser Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_Laser_Cost, "Ability.Phase.Laser.Cost", "To get Phase Laser Ability")
}

namespace MobaPhaseBlackHoleTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_BlackHole, "Ability.Phase.BlackHole", "To get Phase BlackHole Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_BlackHole_Damage, "Ability.Phase.BlackHole.Damage", "To get Phase BlackHole Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_BlackHole_Cooldown, "Ability.Phase.BlackHole.Cooldown", "To get Phase BlackHole Ability")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Phase_BlackHole_Cost, "Ability.Phase.BlackHole.Cost", "To get Phase BlackHole Ability")
}
