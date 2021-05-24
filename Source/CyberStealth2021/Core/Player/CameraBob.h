// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/Rotator.h"
#include "CameraBob.generated.h"

class AStealthPlayerCharacter;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CYBERSTEALTH2021_API UCameraBob : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraBob();

	/**
	* Shifts the Camera position while moving in a "heab-bob" pattern.
	* 
	* This will move the camera up and down in a parabolic pattern, side-to-side with a sin wave, and slightly roll the camera. 
	*
	* @param DeltaTime - Current delta time in seconds.
	*/
	void UpdateCameraBob(float DeltaTime);
	/**
	* Tilt/roll the player camera over specified time.
	* 
	* @param DeltaTime - Current delta time in seconds.
	* @param TiltAmount - The rotation amount you wish to tilt the camera.
	* @param TransitionSpeed - How fast you want to the camera to reach the final TiltAmount. Higher value is quicker, lower value is slower.
	*/
	void TiltPlayerCamera(float DeltaTime, float TiltAmount, float TransitionSpeed);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	AStealthPlayerCharacter *PlayerRef;

	UPROPERTY(EditAnywhere, Category = "HeadBob")
	bool bEnableBob = true;
	// Time between "steps". Higher number is a quicker step. This determines the frequency of the Abs Sin function. 
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float StepFrequency = 6.0f;
	// How much variation there should be in step time for each step, as a percentage. Must be a number between 0 and 100. 0 Means no variation.
	// For example, if StepFrequency was 3, a StepVariation of 20.0 means the final StepFrequency could be up to 20% higher or lower than 3.
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float StepVariation = 35.0f;
	// How extreme the amount of vertical camera movement should be.
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float zHeightMult = 3.2f;
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float zHeightVariation = 40.0f;
	// How much sway from side to side should there be while walking?
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float SwayAmount = 2.0f;
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float SwayVariation = 10.0f;
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float BobRollAmount = 0.25f;
	UPROPERTY(EditAnywhere, Category = "HeadBob")
	float BobRollVariation = 25.0f;

	float VelMultiplier = 0.0f;
	float FadeOut = 0.0f;
	float RandomizedStepFrequency = StepFrequency;
	float RandomizedZHeightMult = zHeightMult;
	float RandomizedSwayAmount = SwayAmount;
	float RandomizedBobRollAmount = BobRollAmount;
	float Offset = 0.0f;
	float zPos = 0.0f;
	float oldZPos = 0.0f;
	float oldOldZPos = 0.0f;
	float yPos = 0.0f;
	float oldYPos = 0.0f;
};
