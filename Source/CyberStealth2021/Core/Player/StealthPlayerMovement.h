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

	// Crouching
	FTimeline CharacterResizeTimeline;
	float CachedHeight = 0.0f;
	float CachedEyeHeight = 0.0f;
	float TargetHeight = 0.0f;
	float TargetEyeHeight = 0.0f;
	UFUNCTION()
	void CharacterResizeAlphaProgress(float Value);
	UFUNCTION()
	void OnFinishCharacterResize();

	// Sliding
	FTimeline SlideTimeline;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	UCurveFloat* SlideAlphaCurve;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideSpeed = 800.0f;
	UFUNCTION()
	void PlayerSlideAlphaProgress();
	UFUNCTION()
	void OnFinishPlayerSlide();
	bool bDidFinishSlide = false;

protected:
	UPROPERTY(EditAnywhere, Category = "Dynamic Crouch")
	UCurveFloat* CharacterResizeAlphaCurve;

public:
	UStealthPlayerMovement();
	virtual void BeginPlay() override;

	virtual void Crouch(bool bClientSimulation) override;
	virtual void UnCrouch(bool bClientSimulation) override;
	virtual float GetMaxSpeed() const override;

	/**
	* Sweeps a box from the center of the player (regardless of their current height) to ensure that there is ample room for them to stand up.
	*
	* @return True if the player has the available height to stand up, False otherwise.
	*/
	UFUNCTION(BlueprintCallable)
	bool CanUncrouch();

	UFUNCTION(BlueprintCallable)
	bool GetInSlideState() { return movementStates.IsInState<PlayerMovementStates::Slide>(); }
	UFUNCTION(BlueprintCallable)
	bool GetInSprintState() { return movementStates.IsInState<PlayerMovementStates::Sprint>(); }
	FVector SlideStartCachedVector;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideTurnReduction = 2.5f;

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
	* Cast a box trace to check if the character needs to adjust their capsule for a new variable crouch height. 
	* 
	* Variable crouch is a feature where the player can crouch into spaces smaller than their regular crouch height. The player
	* does so by simply moving up to a new space while in crouch mode. This function checks if the player is about to 
	* enter a new "variable height" crouch space - either one larger or smaller than the current crouch space, but always smaller than a "regular" crouch space. 
	* 
	* @oaram OutCeilingDistance - provide a float that will be filled with the distance between the old and new ceilings of the two crouch spaces
	* @return True if the player needs to enter a new variable crouch height, False otherwise. 
	*/
	bool CheckNeedsVariableCrouch(float& OutCeilingDistance);

	/**
	* Checks if the player should exist variable crouch entirely, because they are no longer in a "variable height" crouch space. 
	* 
	* @return True if the player should exist variable crouch, False otherwise. 
	*/
	bool CheckCanExitVariableCrouch();

	/**
	* Adjusts the character size smoothly over time to a new height. An example usage is crouching and uncrouching.
	*
	* @param Duration - How long, in seconds, you would like the smooth transition to take.
	* @param NewCharacterCapsuleHeight - The new half height the character should resize to.
	*/
	void ResizeCharacterHeight(float Duration, float NewCharacterCapsuleHeight);
};