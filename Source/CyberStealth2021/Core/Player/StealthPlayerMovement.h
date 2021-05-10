// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerMovement.h"
#include "Components/TimelineComponent.h"
#include "PlayerMovementStates.h"
#include "StealthPlayerMovement.generated.h"

class AStealthPlayerCharacter;

/**
 * 
 */
UCLASS()
class CYBERSTEALTH2021_API UStealthPlayerMovement : public UPBPlayerMovement
{
	GENERATED_BODY()
private:
	friend PlayerMovementStates;
	hsm::StateMachine movementStates;

	AStealthPlayerCharacter *PlayerRef;
	FTimeline CharacterResizeTimeline;
	float CachedHeight = 0.0f;
	float CachedEyeHeight = 0.0f;
	float TargetHeight = 0.0f;
	float TargetEyeHeight = 0.0f;
	UFUNCTION()
	void CharacterResizeAlphaProgress(float Value);
	UFUNCTION()
	void OnFinishCharacterResize();

protected:
	UPROPERTY(EditAnywhere, Category = "Dynamic Crouch");
	UCurveFloat* CharacterResizeAlphaCurve;

public:
	UStealthPlayerMovement();
	virtual void BeginPlay() override;

	virtual void Crouch(bool bClientSimulation) override;
	virtual void UnCrouch(bool bClientSimulation) override;
	virtual float GetMaxSpeed() const override;

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

	/**
	* Sweeps a box from the center of the player (regardless of their current height) to ensure that there is ample room for them to stand up.
	*
	* @return True if the player has the available height to stand up, False otherwise.
	*/
	bool CanUncrouch();

	/**
	* Adjusts the character size smoothly over time to a new height. An example usage is crouching and uncrouching.
	*
	* @param Duration - How long, in seconds, you would like the smooth transition to take.
	* @param NewCharacterCapsuleHeight - The new half height the character should resize to.
	*/
	void ResizeCharacterHeight(float Duration, float NewCharacterCapsuleHeight);
};