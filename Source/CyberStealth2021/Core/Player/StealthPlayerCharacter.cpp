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

	CameraBobComponent = CreateDefaultSubobject<UCameraBob>(TEXT("CameraBobber"));
}

void AStealthPlayerCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!StealthMovementPtr->GetInSlideState()) {
		CameraBobComponent->UpdateCameraBob(DeltaTime);
		
		if (GetInputAxisValue("MoveRight") > 0) {
			CameraBobComponent->TiltPlayerCamera(DeltaTime, 6.0f, 4.0f);
		}
		else if (GetInputAxisValue("MoveRight") < 0) {
			CameraBobComponent->TiltPlayerCamera(DeltaTime, -6.0f, 4.0f);
		}
		else {
			CameraBobComponent->TiltPlayerCamera(DeltaTime, 0.0f, 5.0f);
		}
	}
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
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Dot Product %f"), (1 - dotProduct)));
		//AddControllerYawInput((value * XMouseSensitivity) * (1 / ((StealthMovementPtr->SlideTurnReduction * FMath::Lerp(0, 6, (1 - dotProduct))) + 1)));
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