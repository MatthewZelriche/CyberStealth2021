// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerMovement.h"
#include "StealthPlayerMovement.generated.h"

/**
 * 
 */
UCLASS()
class CYBERSTEALTH2021_API UStealthPlayerMovement : public UPBPlayerMovement
{
	GENERATED_BODY()
public:
	UStealthPlayerMovement();

protected:
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	* Switches between flat vs rounded base for the player collision capsule based on whether the player is approaching a steep ledge.
	*
	* If this function detects that the player has entered the vicinity of a steep ledge, it sets
	* bUseFlatBaseForFloorChecks to true. This allows the player to partially "step off" the ledge without sliding off.
	* If the player is not near a steep ledge, the default behavior is to use a rounded base for their capsule collision,
	* to allow smooth movement when stepping up on small ledges and climbing stairs.
	*/
	void FlatBaseToggle();


	/**
	* Tests for the presence of a floor below the player.
	*
	* @param zOffset - Optionally, you can specify an additional offset to check further below the players feet. By default this is 0.
	* @return True if floor detected, False otherwise.
	*/
	bool TraceTestForFloor(float zOffset);
};
