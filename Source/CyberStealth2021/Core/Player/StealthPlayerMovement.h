// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.
// The function CalculateLeanModifier() is adapted from Dave Watts' CheekyFPS, licensed under the MIT License. Please see the included NOTICE.md for a copy of the license.

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

	UPROPERTY(EditAnywhere, Category = "Sprinting")
	float CrouchToSprintTime = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Crouching")
	float VariableCrouchTime = 15.0f;

	float NewCapsuleHeight = 68.0f;
	float HeightTransitionSpeed = 0.0f;

	float TargetLeanHorzOffset = 0.0f;
	float TargetLeanVertOffset = 0.0f;
	float TargetLeanRot = 0.0f;
	float LeanTransitionSpeed = 0.0f;

	// Sliding
	FTimeline SlideTimeline;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	UCurveFloat* SlideAlphaCurve;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideSpeed = 800.0f;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideHeight = 28.0f;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideTransitionTime = 10.0f;
	UFUNCTION()
	void PlayerSlideAlphaProgress();
	UFUNCTION()
	void OnFinishPlayerSlide();
	bool bDidFinishSlide = false;

public:
	UStealthPlayerMovement();
	virtual void BeginPlay() override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);

	/** The Crouch and UnCrouch functions are overridden but left empty in order to disable PBPlayerMovement crouching logic in favor of our own. */
	virtual void Crouch(bool bClientSimulation) override;
	virtual void UnCrouch(bool bClientSimulation) override;

	/**
	* Gets the current max speed of the character based on the PlayerMovementState they are currently in. 
	* Falls back to PBPlayerMovement behavior if we can't determine the correct speed.
	*
	* @return The current max speed of the player.
	*/
	virtual float GetMaxSpeed() const override;

	/**
	* Sweeps a box from the center of the player (regardless of their current height) to ensure that there is ample room for them to stand up.
	*
	* @return True if the player has the available height to stand up, False otherwise.
	*/
	UFUNCTION(BlueprintCallable)
	bool CanUncrouch();
	/**
	* Get the distance between the bottom of the character capsule and the floor.
	*
	* The default UE4 character controller, which this is expanding upon, floats the character capsule a few
	* units above the floor by design. This function can provide exactly how far above the floor the character
	* is floating, a useful value for certain accurate trace operations.
	*/
	float GetFloorOffset();
	/**
	* Request a new capsule size for the character, for example when entering crouch or setting a new variable crouch height.
	*
	* @param NewSize - The new capsule size for the character, in half height units.
	* @param Speed - How quickly the transition to the new height should be. Lower value is slower, higher value is faster.
	*/
	void RequestCharacterResize(float NewSize, float Speed);

	/**
	* Request a new lean position for the character. 
	*
	* @param HorzOffsetAmount - How many units the character camera should be horizontally offset from its current position (usually 0).
	* @param VertOffsetAmount - How many units the character camera should be vertically offset from its current position (usually 0). 
	* @param CameraRotation - Specify a new Camera Roll rotation.
	* @param TransitionSpeed - How fast should the character transition into this new lean position. Higher values are quicker.
	*/
	UFUNCTION(BlueprintCallable)
	void RequestLean(float HorzOffsetAmount, float VertOffsetAmount, float CameraRotation, float TransitionSpeed);

	UFUNCTION(BlueprintCallable)
	bool GetInSlideState() { return movementStates.IsInState<PlayerMovementStates::Slide>(); }
	UFUNCTION(BlueprintCallable)
	bool GetInSprintState() { return movementStates.IsInState<PlayerMovementStates::Sprint>(); }
	UFUNCTION(BlueprintCallable)
	bool GetInGenericLocomotionState() { return movementStates.IsInState<PlayerMovementStates::GenericLocomotion>(); }
	FVector SlideStartCachedVector;
	UPROPERTY(EditAnywhere, Category = "Sliding")
	float SlideTurnReduction = 2.5f;

protected:
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Called every tick to adjust the player height based on new requested height values from RequestCharacterResize() */
	void UpdateCharacterHeight();
	/** Called every tick to adjust the lean amount based on new lean values from RequestLean() */
	void UpdateLeanState();

private:

	/**
	* Determines much of a requested lean can be performed without camera collisions with geometry.
	* 
	* @return An absolute value between 0.0 and 1.0. This value represents the amount of the originally requested lean that can be satisfied without collisions. 
	* For example, if the function returns 0.75, then we can perform 75% of the requested lean amount without collisions.
	*/
	float CalculateLeanModifier();

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
};