// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "StealthPlayerCharacter.h"

UStealthPlayerMovement::UStealthPlayerMovement() {
	// We want this off by default, so the player can smoothly move up and down steps.
	bUseFlatBaseForFloorChecks = false;

	movementStates.Initialize<PlayerMovementStates::Walk>(this);

	PlayerRef = Cast<AStealthPlayerCharacter>(GetOwner());
}

void UStealthPlayerMovement::BeginPlay() {
	check(CharacterResizeAlphaCurve);

	FOnTimelineFloat ResizeTimelineProgress;
	FOnTimelineEvent FinishedResizeEvent;
	ResizeTimelineProgress.BindUFunction(this, "CharacterResizeAlphaProgress");
	FinishedResizeEvent.BindUFunction(this, "OnFinishCharacterResize");
	CharacterResizeTimeline.AddInterpFloat(CharacterResizeAlphaCurve, ResizeTimelineProgress);
	CharacterResizeTimeline.SetTimelineFinishedFunc(FinishedResizeEvent);
}

void UStealthPlayerMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	movementStates.ProcessStateTransitions();
	FlatBaseToggle();
	CharacterResizeTimeline.TickTimeline(DeltaTime);
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
	else if (movementStates.IsInState <PlayerMovementStates::Slide>()) {
		// Set to 0 and take direct control? Or have a seperate SlideSpeed var?
		return 0.0f;
	}

	// Fallback on Super if no other match found.
	return Super::GetMaxSpeed();
}

void UStealthPlayerMovement::Crouch(bool bClientSimulation) {
	
}

void UStealthPlayerMovement::UnCrouch(bool bClientSimulation) {
	
}

void UStealthPlayerMovement::ResizeCharacterHeight(float Duration, float NewCharacterCapsuleHeight) {
	// Dividing gives us the correct duration in seconds for this function.
	CharacterResizeTimeline.SetPlayRate(1 / Duration);

	// We only want to define new values if we aren't currently in an existing resize.
	if (!CharacterResizeTimeline.IsPlaying()) {
		CachedHeight = GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		CachedEyeHeight = PlayerRef->GetPlayerCamera()->GetRelativeLocation().Z;
		float HeightDiff = CachedHeight - NewCharacterCapsuleHeight;
		TargetEyeHeight = CachedHeight - HeightDiff;
		TargetHeight = NewCharacterCapsuleHeight;
	}

	// If we recieved a resize request during an existing resize, we just reverse the current timeline.
	// This technically could cause problem if you try to VariableCrouch under different ceilings in rapid succession,
	// But level design could be set up to prevent this from happening. 
	if (CharacterResizeTimeline.IsPlaying()) {

		if (CharacterResizeTimeline.IsReversing()) {
			CharacterResizeTimeline.Play();
		}
		else {
			CharacterResizeTimeline.Reverse();
		}
	}
	else {
		CharacterResizeTimeline.PlayFromStart();
	}
}

void UStealthPlayerMovement::CharacterResizeAlphaProgress(float Value) {
	GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(FMath::Lerp(CachedHeight, TargetHeight, Value)); \
	UCameraComponent* camera = PlayerRef->GetPlayerCamera();
	camera->SetRelativeLocation(FVector(camera->GetRelativeLocation().X, camera->GetRelativeLocation().Y, FMath::Lerp(CachedEyeHeight, TargetEyeHeight, Value)));
}

void UStealthPlayerMovement::OnFinishCharacterResize() {
	CachedEyeHeight = PlayerRef->GetPlayerCamera()->GetRelativeLocation().Z;

	// Round out the capsule so we don't have ugly floating point numbers.
	if (FMath::IsNearlyEqual(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), CrouchedHalfHeight, 0.001f)) {
		GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(FMath::RoundToInt(CrouchedHalfHeight));
	}
}

bool UStealthPlayerMovement::CanUncrouch() {
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(10, 10, 0));
	
	FVector Start = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();
	FVector End = Start;
	End.Z = End.Z + PlayerRef->StandingHeight;

	FHitResult discard;		// For now, we never actually need the hit result, so we are discarding it.
	if (GetWorld()->SweepSingleByChannel(discard, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, Box)) {
		return false;
	}
	else {
		return true;
	}
}