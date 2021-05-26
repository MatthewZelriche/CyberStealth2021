// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "StealthPlayerCharacter.h"

UStealthPlayerMovement::UStealthPlayerMovement() {
	// We want this off by default, so the player can smoothly move up and down steps.
	bUseFlatBaseForFloorChecks = false;
	CrouchTime = 6.0f;
	UncrouchTime = 6.0f;
	CrouchedHalfHeight = 42.0f;

	movementStates.Initialize<PlayerMovementStates::GenericLocomotion>(this);

	PlayerRef = Cast<AStealthPlayerCharacter>(GetOwner());
}

void UStealthPlayerMovement::BeginPlay() {
	check(SlideAlphaCurve);

	FOnTimelineFloat SlideTimelineProgress;
	FOnTimelineEvent FinishedSlideEvent;
	SlideTimelineProgress.BindUFunction(this, "PlayerSlideAlphaProgress");
	FinishedSlideEvent.BindUFunction(this, "OnFinishPlayerSlide");
	SlideTimeline.AddInterpFloat(SlideAlphaCurve, SlideTimelineProgress);
	SlideTimeline.SetTimelineFinishedFunc(FinishedSlideEvent);
	SlideTimeline.SetPlayRate(1 / 1.0f);
}

void UStealthPlayerMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCharacterHeight();

	movementStates.ProcessStateTransitions();
	movementStates.UpdateStates();
	FlatBaseToggle();
	SlideTimeline.TickTimeline(DeltaTime);
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
	FVector Start = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();
	FVector End = Start;
	// Subtract any specified user offset, and subtract the units that the character floats above the floor.
	End.Z = End.Z - CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - zOffset - GetFloorOffset();

	FHitResult discard;		// For now, we never actually need the hit result, so we are discarding it.
							// TODO: Think of a way to optionally return this? Function overloading? Out parameters?
	if (GetWorld()->LineTraceSingleByChannel(discard, Start, End, ECollisionChannel::ECC_Visibility)) {
		return true;
	}
	else {
		return false;
	}
}

float UStealthPlayerMovement::GetMaxSpeed() const {
	if (bCheatFlying) {
		return SprintSpeed * 1.5f;
	}
	else if (movementStates.IsInState<PlayerMovementStates::Walk>()) {
		return WalkSpeed;
	} 
	else if (movementStates.IsInState<PlayerMovementStates::Sprint>()) {
		return SprintSpeed;
	}
	else if (movementStates.IsInState<PlayerMovementStates::Crouch>() || movementStates.IsInState<PlayerMovementStates::VariableCrouch>()) {
		return MaxWalkSpeedCrouched;
	} 
	else if (movementStates.IsInState<PlayerMovementStates::Slide>()) {
		return SlideSpeed;
	}

	// Fallback on Super if no other match found.
	return Super::GetMaxSpeed();
}

float UStealthPlayerMovement::GetFloorOffset() {
	FFindFloorResult result;
	FindFloor(CharacterOwner->GetCapsuleComponent()->GetComponentLocation(), result, true);

	
	return result.FloorDist;
}

void UStealthPlayerMovement::Crouch(bool bClientSimulation) {
	
}

void UStealthPlayerMovement::UnCrouch(bool bClientSimulation) {
	
}

void UStealthPlayerMovement::RequestCharacterResize(float NewSize, float Speed) {
	NewCapsuleHeight = NewSize;
	HeightTransitionSpeed = Speed;
}

void UStealthPlayerMovement::UpdateCharacterHeight() {
	float currentHalfHeight = PlayerRef->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	float resizeProgress = FMath::FInterpTo(currentHalfHeight, NewCapsuleHeight, GetWorld()->GetDeltaSeconds(), HeightTransitionSpeed);
	if (FMath::IsNearlyEqual(resizeProgress, NewCapsuleHeight, 0.1f)) {
		resizeProgress = NewCapsuleHeight;
	}
	PlayerRef->GetCapsuleComponent()->SetCapsuleHalfHeight(resizeProgress);
	USpringArmComponent* cameraAnchor = PlayerRef->GetCameraAnchor();

	if (resizeProgress != NewCapsuleHeight) {
		float cameraMoveAmount = currentHalfHeight - resizeProgress;
		cameraAnchor->MoveComponent(FVector(0.0f, 0.0f, -cameraMoveAmount), cameraAnchor->GetRelativeRotation(), false);
	}
}

void UStealthPlayerMovement::PlayerSlideAlphaProgress() {
	PlayerRef->AddMovementInput(PlayerRef->GetActorForwardVector(), 1.0f);
}

void UStealthPlayerMovement::OnFinishPlayerSlide() {
	bDidFinishSlide = true;
}

bool UStealthPlayerMovement::CheckNeedsVariableCrouch(float& OutCeilingDistance) {
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(30, 30, 0));
	static float oldCeilingHeight = 0.0f;

	FVector Start = GetCharacterOwner()->GetCapsuleComponent()->GetComponentLocation();
	Start.Z -= GetFloorOffset();
	FVector End = Start;
	Start.Z = Start.Z - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 1;
	End.Z = (End.Z - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) + (CrouchedHalfHeight * 2);

	FHitResult Result;
	if (GetWorld()->SweepSingleByChannel(Result, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, Box)) {
		OutCeilingDistance = Result.Distance;
		// Don't allow crouching below the minimum allowed crouch size.
		if (OutCeilingDistance < (28.0f * 2)) {
			oldCeilingHeight = 0.0f;
			OutCeilingDistance = 0.0f;
			return false;
		}
		if (!FMath::IsNearlyEqual(OutCeilingDistance, oldCeilingHeight, 0.9f)) {
			oldCeilingHeight = OutCeilingDistance;
			return true;
		}
		else {
			oldCeilingHeight = OutCeilingDistance;
			return false;
		}
	}
	else {
		oldCeilingHeight = 0.0f;
		OutCeilingDistance = 0.0f;
		return false;
	}
}

bool UStealthPlayerMovement::CheckCanExitVariableCrouch() {
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(30, 30, 0));

	FVector Start = GetCharacterOwner()->GetCapsuleComponent()->GetComponentLocation();
	FVector End = Start;
	Start.Z = Start.Z - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	End.Z = (End.Z - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) + (CrouchedHalfHeight * 2);

	FHitResult Result;
	if (GetWorld()->SweepSingleByChannel(Result, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, Box)) {
		return false;
	}
	else {
		return true;
	}
}

bool UStealthPlayerMovement::CanUncrouch() {
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(10, 10, 0));
	
	FVector Start = GetCharacterOwner()->GetCapsuleComponent()->GetComponentLocation();
	Start.Z = Start.Z - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FVector End = Start;
	End.Z = End.Z + (PlayerRef->StandingHeight * 2);

	FHitResult discard;		// For now, we never actually need the hit result, so we are discarding it.
	if (GetWorld()->SweepSingleByChannel(discard, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, Box)) {
		return false;
	}
	else {
		return true;
	}
}