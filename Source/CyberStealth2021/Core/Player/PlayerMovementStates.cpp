#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Misc/App.h"
#include "CollisionQueryParams.h"

hsm::Transition PlayerMovementStates::GenericLocomotion::GetTransition() {
	// bDidFinishSlide is set after the slide timeline is completed. We flip it back to false here until the next slide occurs. 
	if (Owner().bDidFinishSlide) {
		Owner().bDidFinishSlide = false;
		Owner().PlayerRef->Crouch(false);
		Owner().PBCharacter->bIsCrouched = true;
		float OutCeilingDist = 0.0f;
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().VariableCrouchTime);
			return hsm::InnerEntryTransition<VariableCrouch>();
		}
		else {
			Owner().RequestCharacterResize(Owner().CrouchedHalfHeight, Owner().UncrouchTime);
			return hsm::InnerEntryTransition<Crouch>();
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
	Owner().PlayerRef->GetCameraBobber()->TiltPlayerCamera(FApp::GetDeltaTime(), -10.0f, 8.0f);
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
		Owner().PBCharacter->bIsCrouched = true;
		float OutCeilingDist = 0.0f;
		// Enter Variable Crouch State
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().CrouchTime);
			return hsm::SiblingTransition<VariableCrouch>();
		}
		// Enter Regular Crouch State.
		else {
			Owner().RequestCharacterResize(Owner().CrouchedHalfHeight, Owner().CrouchTime);
			return hsm::SiblingTransition<Crouch>();
		}
	}
	// Enter Sprint State
	else if (Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Sprint>();
	}

	return hsm::NoTransition();
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
		Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().VariableCrouchTime);
		return hsm::SiblingTransition<VariableCrouch>();
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
		Owner().RequestCharacterResize(Owner().CrouchedHalfHeight, Owner().VariableCrouchTime);
		return hsm::SiblingTransition<Crouch>();
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
		Owner().RequestCharacterResize(OutCeilingDist / 2.0f, Owner().VariableCrouchTime);
	}
}

hsm::Transition PlayerMovementStates::Sprint::GetTransition() {
	// Enter Walk State
	if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}