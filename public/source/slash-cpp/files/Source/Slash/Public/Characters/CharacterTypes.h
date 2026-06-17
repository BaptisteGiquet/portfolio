#pragma once

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_HitReacting UMETA(DisplayName = "HitReacting"),
	ECS_Attacking UMETA(DisplayName = "Attacking"),
	ECS_Dodging UMETA(DisplayName = "Dodging"),
	
	ECS_Dead UMETA(DisplayName = "Dead")
};


//uint8 is changing the base int32 type of enums for lighter int8
UENUM(BlueprintType)
enum class EEquipmentState : uint8
{
	//UMETA(DisplayName) to have a simplier name in blueprint
	EES_Unequipped UMETA(DisplayName = "Unequipped"),
	EES_EquippedOneHandedWeapon UMETA(DisplayName = "Equipped One Handed Weapon"),
	EES_EquippedTwoHandedWeapon UMETA(DisplayName = "Equipped Two Handed Weapon")
};

UENUM(BlueprintType)
enum class EDeadPose : uint8
{
	EDP_Dead1 UMETA(DisplayName = "Dead1"),
	
	EDP_COUNT UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EEnemyStates : uint8
{
	EES_NoState UMETA(DisplayName = "NoState"),
	EES_Patrolling UMETA(DisplayName = "Patrolling"),
	EES_Chasing UMETA(DisplayName = "Chasing"),
	EES_Attacking UMETA(DisplayName = "Attacking"),
	EES_Engaged UMETA(DisplayName = "Engaged"),

	EES_Dead UMETA(DisplayName = "Dead")
};