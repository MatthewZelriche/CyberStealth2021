// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerCharacter.h"
#include "StealthPlayerMovement.h"

AStealthPlayerCharacter::AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UStealthPlayerMovement>(ACharacter::CharacterMovementComponentName)) {

}