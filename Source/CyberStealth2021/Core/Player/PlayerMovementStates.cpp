#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Misc/App.h"
#include "CollisionQueryParams.h"

hsm::Transition PlayerMovementStates::GenericLocomotion::GetTransition() {
	// bDidFinishSlide is set after the slide timeline is completed. We flip it back to false here until the next slide occurs. 
	if (Owner().bDidFinishSlide) {
		Owner().bDidFinishSlide = false;
		float OutCeilingDist = 0.0f;
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			return hsm::InnerEntryTransition<VariableCrouch>(true);
		}
		else {
			return hsm::InnerEntryTransition<Crouch>(Owner().UncrouchTime, true);
		}
	}
	else if (Owner().PBCharacter->IsSprinting()) {
		if (Owner().bWantsToCrouch && !(Owner().movementStates.IsInState<Crouch>() || Owner().movementStates.IsInState<VariableCrouch>())) {
			return hsm::SiblingTransition<Slide>();
		}
		else {
			return hsm::InnerEntryTransition<Sprint>();
		}
	}
	else if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::InnerEntryTransition<Walk>();
	}
	else {
		return hsm::NoTransition();
	}
}

hsm::Transition PlayerMovementStates::Slide::GetTransition() {
	if (Owner().bDidFinishSlide || CheckIfSlideInterrupted()) {
		return hsm::SiblingTransition<GenericLocomotion>();
	}
	else {
		return hsm::NoTransition();
	}
}

void PlayerMovementStates::Slide::Update() {
	Owner().PlayerRef->GetCameraFXHandler()->TiltPlayerCamera(FApp::GetDeltaTime(), -10.0f, 8.0f);
}

void PlayerMovementStates::Slide::OnEnter() {
	Owner().SlideStartCachedVector = Owner().PlayerRef->GetActorForwardVector();
	Owner().RequestCharacterResize(Owner().SlideHeight, Owner().SlideTransitionTime);
	Owner().SlideTimeline.PlayFromStart();
}

void PlayerMovementStates::Slide::OnExit() {
	Owner().SlideTimeline.Stop();
}

bool PlayerMovementStates::Slide::CheckIfSlideInterrupted() {
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(15, 15, 0));
	FVector Start = Owner().PlayerRef->GetCapsuleComponent()->GetComponentLocation();
	Start = Start + (Owner().PlayerRef->GetActorForwardVector() * 20.0f);
	Start.Z = Start.Z - Owner().GetFloorOffset() - Owner().PlayerRef->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 1;
	FVector End = Start;
	Start.Z = Start.Z + Owner().MaxStepHeight;
	End.Z = End.Z + (56);

	FHitResult Result;
	// FCollisionQueryParams::DefaultQueryParam is set to trace complex collision. For some reason, that is causing the trace to fail
	// for me. So for now I have to substitute my own default FCollisionQueryParams that isn't tracing for complex collision.
	static FCollisionQueryParams defaultParams;
	if (Owner().GetWorld()->SweepSingleByChannel(Result, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, Box, defaultParams)) {
		return true;
	}
	else {
		return false;
	}
}

hsm::Transition PlayerMovementStates::Walk::GetTransition() {
	if (Owner().bWantsToCrouch && Owner().CharacterOwner->CanCrouch()) {
		float OutCeilingDist = 0.0f;
		// Enter Variable Crouch State
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			return hsm::SiblingTransition<VariableCrouch>(false, true);
		}
		// Enter Regular Crouch State.
		else {
			return hsm::SiblingTransition<Crouch>(Owner().CrouchTime);
		}
	}
	// Enter Sprint State
	else if (Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Sprint>();
	}

	return hsm::NoTransition();
}

void PlayerMovementStates::Crouch::OnEnter(float TransitionSpeed, bool bForceCrouch) {
	if (bForceCrouch) {
		Owner().PlayerRef->Crouch(false);
	}
	Owner().PBCharacter->bIsCrouched = true;
	Owner().RequestCharacterResize(Owner().CrouchedHalfHeight, TransitionSpeed);
}

void PlayerMovementStates::Crouch::OnExit() {
	Owner().PBCharacter->bIsCrouched = false;
}

void PlayerMovementStates::VariableCrouch::OnEnter(bool bForceCrouch, bool bRegularCrouchSpeed) {
	if (bForceCrouch) {
		Owner().PlayerRef->Crouch(false);
	}
	Owner().PBCharacter->bIsCrouched = true;

	mRegularCrouchSpeed = bRegularCrouchSpeed;
	float OutCeilingDist = 0.0f;
	Owner().CheckNeedsVariableCrouch(OutCeilingDist);
	EntryHeight = OutCeilingDist / 2;
}

hsm::Transition PlayerMovementStates::Crouch::GetTransition() {
	float OutCeilingDist = 0.0f;
	// Enter Sprint State
	if (Owner().PBCharacter->IsSprinting()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().PlayerRef->UnCrouch();
		Owner().RequestCharacterResize(Owner().PlayerRef->StandingHeight, Owner().CrouchToSprintTime);
		return hsm::SiblingTransition<Sprint>();
	}
	// Enter Walk State
	else if (!Owner().bWantsToCrouch && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().RequestCharacterResize(Owner().PlayerRef->StandingHeight, Owner().UncrouchTime);
		return hsm::SiblingTransition<Walk>();
	}
	
	// Enter Variable Crouch State
	if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
		return hsm::SiblingTransition<VariableCrouch>(false);
	}

	return hsm::NoTransition();
}

hsm::Transition PlayerMovementStates::VariableCrouch::GetTransition() {
	float OutCeilingDist = 0.0f;

	// Enter Sprint State
	if (Owner().PBCharacter->IsSprinting() && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().PlayerRef->UnCrouch();
		Owner().RequestCharacterResize(Owner().PlayerRef->StandingHeight, Owner().CrouchToSprintTime);
		return hsm::SiblingTransition<Sprint>();
	}
	// Enter Regular Crouch State
	else if (Owner().bWantsToCrouch && Owner().CheckCanExitVariableCrouch()) {
		return hsm::SiblingTransition<Crouch>(Owner().VariableCrouchTime);
	}
	// Enter Walk State
	else if (!Owner().bWantsToCrouch && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().RequestCharacterResize(Owner().PlayerRef->StandingHeight, Owner().UncrouchTime);
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}

void PlayerMovementStates::VariableCrouch::Update() {
	float OutCeilingDist = 0.0f;
	if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
		if (!mRegularCrouchSpeed) {
			Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().VariableCrouchTime);
		}
		// This block is for if we transitioned into this code with the request for the very first resize to be done
		// at a regular crouch speed instead of a variable crouch speed. This code is ugly but I couldn't think of a 
		// better way of doing this without a large rewrite. 
		// Currently, this is only for transitioning from Walk -> VariableCrouch directly, so that the transition doesn't make the 
		// player go straight from standing to a full variable crouch in a really short amount of time. 
		else {
			if (!FMath::IsNearlyEqual(EntryHeight, Owner().PlayerRef->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 0.9f)) {
				if (EntryHeight != -1.0f) {
					Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().CrouchTime);
				}
				else {
					Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().VariableCrouchTime);
				}
			}
			else {
				EntryHeight = -1.0f;
			}
		}
	}
}

hsm::Transition PlayerMovementStates::Sprint::GetTransition() {
	// Enter Walk State
	if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}

void PlayerMovementStates::Sprint::OnEnter() {
	float currentFOV = Owner().PlayerRef->GetPlayerCamera()->FieldOfView;
	Owner().PlayerRef->GetCameraFXHandler()->RequestNewFOV(currentFOV + Owner().PlayerRef->GetCameraFXHandler()->GetSprintFOVOffset(), SprintFOVTransitionSpeed);
}

void PlayerMovementStates::Sprint::OnExit() {
	float DefaultFOV = Owner().PlayerRef->GetCameraFXHandler()->GetCameraDefaultFOV();
	Owner().PlayerRef->GetCameraFXHandler()->RequestNewFOV(DefaultFOV, SprintFOVTransitionSpeed);
}