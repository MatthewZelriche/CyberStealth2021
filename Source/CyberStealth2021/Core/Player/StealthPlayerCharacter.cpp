// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerCharacter.h"
#include "StealthPlayerMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

AStealthPlayerCharacter::AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UStealthPlayerMovement>(ACharacter::CharacterMovementComponentName)) {

	GetCapsuleComponent()->InitCapsuleSize(28.0f, StandingHeight);

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(GetCapsuleComponent());
	PlayerCamera->SetRelativeLocation(FVector(0, 0, StandingEyeHeight));
	PlayerCamera->bUsePawnControlRotation = true;
}

void AStealthPlayerCharacter::Sprint() {
	bIsSprinting = true;
}

void AStealthPlayerCharacter::StopSprinting() {
	bIsSprinting = false;
}