// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerCharacter.h"
#include "StealthPlayerMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

AStealthPlayerCharacter::AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UStealthPlayerMovement>(ACharacter::CharacterMovementComponentName)) {

	GetCapsuleComponent()->InitCapsuleSize(28.0f, StandingHeight);

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(GetCapsuleComponent());
	PlayerCamera->SetRelativeLocation(FVector(0, 0, StandingEyeHeight));
	PlayerCamera->bUsePawnControlRotation = true;

	// Get pointer to overridden movement component
	StealthMovementPtr = Cast<UStealthPlayerMovement>(ACharacter::GetMovementComponent());
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

void AStealthPlayerCharacter::Sprint() {
	bIsSprinting = true;
}

void AStealthPlayerCharacter::StopSprinting() {
	bIsSprinting = false;
}