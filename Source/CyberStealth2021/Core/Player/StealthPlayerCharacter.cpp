// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerCharacter.h"
#include "StealthPlayerMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

AStealthPlayerCharacter::AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UStealthPlayerMovement>(ACharacter::CharacterMovementComponentName)) {

	GetCapsuleComponent()->InitCapsuleSize(28.0f, StandingHeight);

	// We aren't actually using the SpringArm for its intended purpose. It's just an "anchor" object, so that we 
	// can bob the camera without having to worry about things like player's eye height. SpringArm is used specifically
	// because we can use bInheritRoll = false, allowing us to roll the SpringArm but still use control rotation.
	CameraAnchor = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraAnchor"));
	CameraAnchor->SetupAttachment(GetCapsuleComponent());
	CameraAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, StandingEyeHeight));
	CameraAnchor->TargetArmLength = 0.0f;
	CameraAnchor->bEnableCameraLag = false;
	CameraAnchor->bUsePawnControlRotation = true;		// Critical for ensuring capsule doesn't rotate when we look up and down.
	CameraAnchor->bInheritRoll = false;

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraAnchor);
	PlayerCamera->SetRelativeLocation(FVector(0, 0, 0));
	
	// Get pointer to overridden movement component
	StealthMovementPtr = Cast<UStealthPlayerMovement>(ACharacter::GetMovementComponent());

	CameraFXHandler = CreateDefaultSubobject<UCameraFXHandler>(TEXT("CameraFXHandler"));
}

void AStealthPlayerCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// TODO: Doesn't work well with lean camera tilt. How to fix?
	/*
	if (!StealthMovementPtr->GetInSlideState()) {
		if (GetInputAxisValue("MoveRight") > 0) {
			CameraFXHandler->TiltPlayerCamera(DeltaTime, CameraFXHandler->GetStrafeTiltAmount(), 
													CameraFXHandler->GetStrafeTiltEnterTime());
		}
		else if (GetInputAxisValue("MoveRight") < 0) {
			CameraFXHandler->TiltPlayerCamera(DeltaTime, -CameraFXHandler->GetStrafeTiltAmount(), 
													CameraFXHandler->GetStrafeTiltEnterTime());
		}
		else {
			CameraFXHandler->TiltPlayerCamera(DeltaTime, 0.0f, CameraFXHandler->GetStrafeTiltExitTime());
		}
	}
	*/
}

void AStealthPlayerCharacter::OnJumped_Implementation() {
	Super::OnJumped_Implementation();
	bIsAvailableForLedgeGrab = true;
}

void AStealthPlayerCharacter::NotifyJumpApex() {
	bIsAvailableForLedgeGrab = false;
}

void AStealthPlayerCharacter::LookY(float value) {
	if (!StealthMovementPtr->GetInSlideState()) {
		AddControllerPitchInput(value * XMouseSensitivity);
	}
	else {
		float dotProduct = FVector::DotProduct(StealthMovementPtr->SlideStartCachedVector.GetSafeNormal(), GetActorForwardVector().GetSafeNormal());
		AddControllerPitchInput((value * XMouseSensitivity) * (1 / ((StealthMovementPtr->SlideTurnReduction * (1 - dotProduct)) + 1)));
	}
}

void AStealthPlayerCharacter::LookX(float value) {
	if (!StealthMovementPtr->GetInSlideState()) {
		AddControllerYawInput(value * XMouseSensitivity);
	}
	else {
		float dotProduct = FVector::DotProduct(StealthMovementPtr->SlideStartCachedVector.GetSafeNormal(), GetActorForwardVector().GetSafeNormal());
		AddControllerYawInput((value * XMouseSensitivity) * (1 / ((StealthMovementPtr->SlideTurnReduction * (1 - dotProduct)) + 1)));
	}
}

void AStealthPlayerCharacter::Crouch(bool bClientSimulation) {
	StealthMovementPtr->bWantsToCrouch = true;
}

void AStealthPlayerCharacter::OnPlayerStepped() {
	// TODO: Footstep sounds.
}

void AStealthPlayerCharacter::Sprint() {
	bIsSprinting = true;
}

void AStealthPlayerCharacter::StopSprinting() {
	bIsSprinting = false;
}