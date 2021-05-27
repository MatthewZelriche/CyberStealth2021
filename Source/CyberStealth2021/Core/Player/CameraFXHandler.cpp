// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "CameraFXHandler.h"
#include "StealthPlayerCharacter.h"
#include "StealthPlayerMovement.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UCameraFXHandler::UCameraFXHandler()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PlayerRef = Cast<AStealthPlayerCharacter>(GetOwner());
}


// Called when the game starts
void UCameraFXHandler::BeginPlay()
{
	Super::BeginPlay();
	NewFOV = PlayerRef->GetPlayerCamera()->FieldOfView;
}

void UCameraFXHandler::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	if (!PlayerRef->GetStealthMovementComp()->GetInSlideState()) {
		UpdateFOV(DeltaTime);
		UpdateCameraBob(DeltaTime);
	}
}

void UCameraFXHandler::TiltPlayerCamera(float DeltaTime, float TiltAmount, float TransitionSpeed) {
	FRotator newTilt(0.0f, 0.0f, FMath::FInterpTo(PlayerRef->GetCameraAnchor()->GetRelativeRotation().Roll, TiltAmount, DeltaTime, TransitionSpeed));
	PlayerRef->GetCameraAnchor()->SetRelativeRotation(newTilt);
}

void UCameraFXHandler::UpdateFOV(float DeltaTime) {
	if (NewFOV != PlayerRef->GetPlayerCamera()->FieldOfView) {
		PlayerRef->GetPlayerCamera()->FieldOfView = FMath::FInterpTo(PlayerRef->GetPlayerCamera()->FieldOfView, NewFOV, DeltaTime, FOVTransitionSpeed);
	}

	if (FMath::IsNearlyEqual(NewFOV, PlayerRef->GetPlayerCamera()->FieldOfView, 0.1f)) {
		PlayerRef->GetPlayerCamera()->FieldOfView = NewFOV;
	}
}

void UCameraFXHandler::RequestNewFOV(float NewFOVParam, float TransitionSpeed) {
	NewFOV = NewFOVParam;
	FOVTransitionSpeed = TransitionSpeed;
}


void UCameraFXHandler::UpdateCameraBob(float DeltaTime) {
	// TODO: Replace this with a slider that can adjust bob intensity or disable entirely.
	if (!bEnableBob) {
		return;
	}

	float Velocity = FVector(PlayerRef->GetVelocity().X, PlayerRef->GetVelocity().Y, 0).Size();

	if (Velocity != 0.0f) {
		// TODO: Dehardcode min and max speed values
		VelMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(120.0f, 610.0f),
			FVector2D(0.5f, 1.5f), Velocity);

		// Check if we have just placed a foot on the ground (ie, are we at the bottom of the parabolic curve)
		if (((oldOldZPos > oldZPos) && oldZPos < zPos) || zPos == 0.0f) {
			// Use Perlin Noise instead of a totally random value to avoid extreme sudden changes in step frequency.
			RandomizedStepFrequency = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f),
				FVector2D(StepFrequency - StepFrequency * (StepVariation / 100),
					StepFrequency + StepFrequency * (StepVariation / 100)),
				FMath::PerlinNoise1D(Offset));
			RandomizedZHeightMult = FMath::RandRange(zHeightMult - zHeightMult * (zHeightVariation / 100),
				zHeightMult + zHeightMult * (zHeightVariation / 100));
			PlayerRef->OnPlayerStepped();
		}

		// Check if weve finished a cycle for side-to-side or camera roll movement. (The sine functions for these start at 0).
		if (oldYPos <= 0.0f && yPos >= 0.0f) {
			RandomizedSwayAmount = FMath::RandRange(SwayAmount - SwayAmount * (SwayVariation / 100),
				SwayAmount + SwayAmount * (SwayAmount / 100));
			RandomizedBobRollAmount = FMath::RandRange(BobRollAmount - BobRollAmount * (BobRollVariation / 100),
				BobRollAmount + BobRollAmount * (BobRollVariation / 100));
		}
	}
	else {
		VelMultiplier = 0.0f;
	}

	Offset += RandomizedStepFrequency * DeltaTime * VelMultiplier;

	float fadeDirection = Velocity < 0.1f ? 0 : 1;
	FadeOut = FMath::FInterpTo(FadeOut, fadeDirection, DeltaTime, 5);

	oldOldZPos = oldZPos;
	oldZPos = zPos;
	// Add an additional offset so that we start at the top of a parabola, bringing the camera down towards placing the first step.
	zPos = FMath::Abs(FMath::Sin(Offset + 0.5f * 3.141592)) * RandomizedZHeightMult * FadeOut;

	// Don't fade out the side to side movement because it can look weird. Side to side movement shouldnt be that dramatic anyway.
	oldYPos = yPos;
	yPos = FMath::Sin(Offset) * RandomizedSwayAmount;

	FVector newLocation(0, 0, 0);
	newLocation = newLocation + FVector(0.0f, yPos, zPos);
	PlayerRef->GetPlayerCamera()->SetRelativeLocation(newLocation);

	// Create a new Rotator to be added to the player camera rotation.
	float RollAmountThisFrame = FMath::Sin(Offset) * RandomizedBobRollAmount * FadeOut;
	FRotator BobRotation(0.0f, 0.0f, RollAmountThisFrame);
	// Apply specifically to the camera instead of the anchor, so we can layer different rotations on top of each other.
	PlayerRef->GetPlayerCamera()->SetRelativeRotation(BobRotation);
}
