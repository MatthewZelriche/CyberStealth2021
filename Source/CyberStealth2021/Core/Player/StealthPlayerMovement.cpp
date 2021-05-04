// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UStealthPlayerMovement::UStealthPlayerMovement() {
	// We want this off by default, so the player can smoothly move up and down steps.
	bUseFlatBaseForFloorChecks = false;
}

void UStealthPlayerMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FlatBaseToggle();
}

void UStealthPlayerMovement::FlatBaseToggle() {
	// No need to alter base when in midair.
	if (IsMovingOnGround()) {
		// We add max step height to our trace, because we don't want a flat base when the player
		// approaches ledges that they should be able to just "step off".
		if (!TraceTestForFloor(MaxStepHeight)) {
			bUseFlatBaseForFloorChecks = true;
		}
		else {
			bUseFlatBaseForFloorChecks = false;
		}
	}
}

bool UStealthPlayerMovement::TraceTestForFloor(float zOffset = 0) {
	// We need to get the distance to the floor, since the built-in character controller 
	// hovers the capsule slightly above the ground. This distance should never exceed 2.4f while the player is still "grounded".
	FFindFloorResult result;
	FindFloor(CharacterOwner->GetCapsuleComponent()->GetComponentLocation(), result, true);

	FVector Start = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();
	FVector End = Start;

	// Subtract any specified user offset, and subtract the abovementioned units that the character floats above the floor.
	End.Z = End.Z - CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - zOffset - result.FloorDist;

	FHitResult discard;		// For now, we never actually need the hit result, so we are discarding it.
							// TODO: Think of a way to optionally return this? Function overloading? Out parameters?
	if (GetWorld()->LineTraceSingleByChannel(discard, Start, End, ECollisionChannel::ECC_Visibility)) {
		return true;
	}
	else {
		return false;
	}
}