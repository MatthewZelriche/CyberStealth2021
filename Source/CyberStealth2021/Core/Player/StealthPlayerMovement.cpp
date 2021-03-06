// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.


#include "StealthPlayerMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "StealthPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Algo/Reverse.h"

#include "CameraFXHandler.h"

UStealthPlayerMovement::UStealthPlayerMovement() {
	// We want this off by default, so the player can smoothly move up and down steps.
	bUseFlatBaseForFloorChecks = false;
	bNotifyApex = true;
	CrouchTime = 6.0f;
	UncrouchTime = 6.0f;
	CrouchedHalfHeight = 42.0f;

	movementStates.Initialize<PlayerMovementStates::GenericLocomotion>(this);

	PlayerRef = Cast<AStealthPlayerCharacter>(GetOwner());
}

void UStealthPlayerMovement::BeginPlay() {
	Super::BeginPlay();
	check(SlideAlphaCurve);
	check(ClimbAlphaCurve);

	FOnTimelineFloat SlideTimelineProgress;
	FOnTimelineEvent FinishedSlideEvent;
	SlideTimelineProgress.BindUFunction(this, "PlayerSlideAlphaProgress");
	FinishedSlideEvent.BindUFunction(this, "OnFinishPlayerSlide");
	SlideTimeline.AddInterpFloat(SlideAlphaCurve, SlideTimelineProgress);
	SlideTimeline.SetTimelineFinishedFunc(FinishedSlideEvent);
	SlideTimeline.SetPlayRate(1 / 1.0f);

	FOnTimelineFloat ClimbTimelineProgress;
	FOnTimelineEvent FinishedClimbEvent;
	ClimbTimelineProgress.BindUFunction(this, "PlayerClimbAlphaProgress");
	FinishedClimbEvent.BindUFunction(this, "OnFinishedPlayerClimb");
	ClimbTimeline.AddInterpFloat(ClimbAlphaCurve, ClimbTimelineProgress);
	ClimbTimeline.SetTimelineFinishedFunc(FinishedClimbEvent);
	ClimbTimeline.SetPlayRate(1 / 0.3f);

	
}

void UStealthPlayerMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) {
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == EMovementMode::MOVE_Falling) {
		bNotifyApex = true;
	}
}

void UStealthPlayerMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCharacterHeight();
	UpdateLeanState();

	movementStates.ProcessStateTransitions();
	movementStates.UpdateStates();
	FlatBaseToggle();
	SlideTimeline.TickTimeline(DeltaTime);
	ClimbTimeline.TickTimeline(DeltaTime);
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

bool UStealthPlayerMovement::TestForValidLedges(FVector& OutValidLedgeLocation) {
	UCapsuleComponent* playerCapsule = CharacterOwner->GetCapsuleComponent();
	float halfHeight = playerCapsule->GetUnscaledCapsuleHalfHeight();

	// Check in front of the player to see if there are any ledges within the player's "grabbing range". 
	TArray<FHitResult> results;
	FVector start = playerCapsule->GetComponentLocation();
	start.Z = (start.Z + halfHeight) + LedgeGrabHeightAboveHead;
	start = start + (playerCapsule->GetForwardVector() * MaxClimbAngle);
	FVector end = start;
	end.Z = end.Z - (halfHeight * 2) + MaxStepHeight;
	if (!GetWorld()->SweepMultiByObjectType(results, start, end, FQuat::Identity, FCollisionObjectQueryParams::AllStaticObjects, FCollisionShape::MakeSphere(5.0f))) {
		return false;
	}

	// Iterate through each potential ledge, starting from the last (lowest) ledge encountered.
	for (auto potentialLedge : results) {
		if (potentialLedge.Time == 0) {
			return false;
		}
		
		// Check if there's actually enough height above the player to move into the requested ledge position
		FVector enoughHeadroomEnd = playerCapsule->GetComponentLocation();
		enoughHeadroomEnd.Z = enoughHeadroomEnd.Z + ((halfHeight * 2) + potentialLedge.ImpactPoint.Z) - enoughHeadroomEnd.Z;
		FHitResult discard;
		if (GetWorld()->LineTraceSingleByChannel(discard, playerCapsule->GetComponentLocation(), enoughHeadroomEnd, ECollisionChannel::ECC_Visibility)) {
			return false;
		}

		// Capsule Trace at the final ledge position to ensure there's enough room for the player.
		FCollisionShape capsuleCheck = FCollisionShape::MakeCapsule(playerCapsule->GetUnscaledCapsuleRadius(), halfHeight);
		FHitResult CheckSpaceHitResult;
		FVector roomStart = potentialLedge.ImpactPoint;
		roomStart.Z += halfHeight;
		roomStart.Z += 2.0f;		// Buffer for ensuring the trace doesnt touch the floor in an otherwise valid position.
		if (!GetWorld()->SweepSingleByChannel(CheckSpaceHitResult, roomStart, roomStart, FQuat::Identity, ECollisionChannel::ECC_Visibility, capsuleCheck)) {
			// If there's no hit, we are good to go!
			OutValidLedgeLocation = roomStart;
			return true;
		}
		else {
			// If there is a hit, we need to test an edge case to see if the collision is actually just something the player can step up over.
			if (roomStart.Z - halfHeight - CheckSpaceHitResult.ImpactPoint.Z <= MaxStepHeight) {
				FVector EdgeCaseStart = roomStart;
				//EdgeCaseStart.Z += roomStart.Z - halfHeight - CheckSpaceHitResult.ImpactPoint.Z;
				EdgeCaseStart.Z = EdgeCaseStart.Z + (CheckSpaceHitResult.ImpactPoint.Z - (roomStart.Z - halfHeight));
				const FName TraceTag("MyTraceTag");
				GetWorld()->DebugDrawTraceTag = TraceTag;
				FCollisionQueryParams params;
				params.TraceTag = TraceTag;
				if (!GetWorld()->SweepSingleByChannel(CheckSpaceHitResult, EdgeCaseStart, EdgeCaseStart, FQuat::Identity, ECollisionChannel::ECC_Visibility, capsuleCheck, params)) {
					// We've confirmed the edge case and that the player can indeed fit here
					OutValidLedgeLocation = EdgeCaseStart;
					return true;
				}
			}
		}
	}

	// None of the ledges were valid.
	return false;

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
	NewCapsuleHeight = FMath::Floor(NewSize);
	HeightTransitionSpeed = Speed;
}

void UStealthPlayerMovement::RequestLean(float HorzOffsetAmount, float VertOffsetAmount, float CameraRotation, float TransitionSpeed) {
	TargetLeanHorzOffset = HorzOffsetAmount;
	TargetLeanVertOffset = VertOffsetAmount;
	TargetLeanRot = CameraRotation;
	LeanTransitionSpeed = TransitionSpeed;
}

float UStealthPlayerMovement::CalculateLeanModifier() {
	USpringArmComponent* cameraAnchor = PlayerRef->GetCameraAnchor();
	FHitResult result;
	FCollisionShape sphere = FCollisionShape::MakeSphere(25.0f);
	if (GetWorld()->SweepSingleByChannel(result, cameraAnchor->GetComponentLocation(), ((cameraAnchor->GetRightVector() * (TargetLeanHorzOffset)) + cameraAnchor->GetComponentLocation()),
		FQuat::Identity, ECollisionChannel::ECC_Visibility, sphere)) {
		// Convert the distance to a normalized value between 0 and 1.
		return FMath::Abs(result.Distance / (TargetLeanHorzOffset));
	}
	else {
		// We have a full lean space, so theres no need to reduce the amount we actually lean.
		return 1.0f;
	}
}

void UStealthPlayerMovement::UpdateLeanState() {
	USpringArmComponent* cameraAnchor = PlayerRef->GetCameraAnchor();
	static float LastHorzLeanProgress = 0.0f;
	static float LastVertLeanProgress = 0.0f;

	// If there isn't enough space to lean fully (eg, attempting to lean next to a wall) reduce the amount of lean distance appropriately. 
	float LeanMod = CalculateLeanModifier();
	float HorzLeanProgress = FMath::FInterpTo(LastHorzLeanProgress, LeanMod * TargetLeanHorzOffset, GetWorld()->GetDeltaSeconds(), LeanTransitionSpeed);
	float VertLeanProgress = FMath::FInterpTo(LastVertLeanProgress, LeanMod * TargetLeanVertOffset, GetWorld()->GetDeltaSeconds(), LeanTransitionSpeed);

	float VertLeanDelta = 0.0f;
	float HorzLeanDelta = 0.0f;

	
	if (FMath::IsNearlyEqual(VertLeanProgress, TargetLeanVertOffset, 0.1f)) {
		VertLeanProgress = TargetLeanVertOffset;
	}
	else {
		VertLeanDelta = VertLeanProgress - LastVertLeanProgress;
	}

	if (FMath::IsNearlyEqual(HorzLeanProgress, TargetLeanHorzOffset, 0.1f)) {
		HorzLeanProgress = TargetLeanHorzOffset;
	}
	else {
		HorzLeanDelta = HorzLeanProgress - LastHorzLeanProgress;
	}

	// TODO: This in-progress lean rotation breaks the strafe leaning. How to have them work together?
	UCameraFXHandler* cameraFX = PlayerRef->GetCameraFXHandler();
	cameraFX->TiltPlayerCamera(GetWorld()->GetDeltaSeconds(), TargetLeanRot * LeanMod, LeanTransitionSpeed);
	cameraAnchor->AddRelativeLocation(FVector(0.0f, HorzLeanDelta, VertLeanDelta));
	LastHorzLeanProgress = HorzLeanProgress;
	LastVertLeanProgress = VertLeanProgress;
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

void UStealthPlayerMovement::PlayerClimbAlphaProgress() {
	float alpha = ClimbAlphaCurve->GetFloatValue(ClimbTimeline.GetPlaybackPosition());
	PlayerRef->SetActorLocation(FMath::Lerp(StartClimbPos, EndClimbPos, alpha));
}

void UStealthPlayerMovement::OnFinishPlayerSlide() {
	bDidFinishSlide = true;
}

void UStealthPlayerMovement::OnFinishedPlayerClimb() {
	bDidFinishClimb = true;
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
		oldCeilingHeight = OutCeilingDistance;
		return true;
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
	Start.Z = Start.Z - (GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) * GetWorld()->GetDeltaSeconds();
	End.Z = (End.Z - (GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) * GetWorld()->GetDeltaSeconds()) + (CrouchedHalfHeight * 2);

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