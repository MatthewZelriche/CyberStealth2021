#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"

#include "Misc/App.h"

hsm::Transition PlayerMovementStates::GenericLocomotion::GetTransition() {
	// bDidFinishSlide is set after the slide timeline is completed. We flip it back to false here until the next slide occurs. 
	if (Owner().bDidFinishSlide) {
		Owner().bDidFinishSlide = false;
		Owner().PlayerRef->Crouch(false);
		Owner().PBCharacter->bIsCrouched = true;
		float OutCeilingDist = 0.0f;
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			Owner().ResizeCharacterHeight(0.15f, OutCeilingDist / 2.0f);		// Halve the Distance since we are resizing the half-height.
			return hsm::InnerEntryTransition<VariableCrouch>();
		}
		else {
			Owner().ResizeCharacterHeight(Owner().CrouchTime, Owner().CrouchedHalfHeight);
			return hsm::InnerEntryTransition<Crouch>();
		}
	}
	else if (Owner().PBCharacter->IsSprinting()) {
		if (Owner().bWantsToCrouch && !(Owner().movementStates.IsInState<Crouch>() || Owner().movementStates.IsInState<VariableCrouch>())) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Begin slide!"));
			return hsm::SiblingTransition<Slide>();
		}
		else {
			return hsm::InnerEntryTransition<Sprint>();
		}
	}
	else {
		return hsm::InnerEntryTransition<Walk>();
	}
}

hsm::Transition PlayerMovementStates::Slide::GetTransition() {
	if (Owner().bDidFinishSlide) {
		return hsm::SiblingTransition<GenericLocomotion>();
	}
	else {
		return hsm::NoTransition();
	}
}

void PlayerMovementStates::Slide::Update() {
	// TODO: We need to check if the player slides into an object, and abort the slide early if they did.
	Owner().PlayerRef->GetCameraBobber()->TiltPlayerCamera(FApp::GetDeltaTime(), -10.0f, 8.0f);
}

void PlayerMovementStates::Slide::OnEnter() {
	Owner().SlideStartCachedVector = Owner().PlayerRef->GetActorForwardVector();
	Owner().ResizeCharacterHeight(0.2f, 28.0f);
	Owner().SlideTimeline.PlayFromStart();
}

hsm::Transition PlayerMovementStates::Walk::GetTransition() {
	if (Owner().bWantsToCrouch && Owner().CharacterOwner->CanCrouch()) {
		Owner().PBCharacter->bIsCrouched = true;
		float OutCeilingDist = 0.0f;
		// Enter Variable Crouch State
		if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
			Owner().ResizeCharacterHeight(Owner().CrouchTime, OutCeilingDist / 2.0f);		// Halve the Distance since we are resizing the half-height.
			return hsm::SiblingTransition<VariableCrouch>();
		}
		// Enter Regular Crouch State.
		else {
			Owner().ResizeCharacterHeight(Owner().CrouchTime, Owner().CrouchedHalfHeight);
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
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Sprint>();
	}
	// Enter Walk State
	else if (!Owner().bWantsToCrouch && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Walk>();
	}
	
	// Enter Variable Crouch State
	if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
		Owner().ResizeCharacterHeight(0.15f, OutCeilingDist / 2.0f);		// Halve the Distance since we are resizing the half-height.
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
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Sprint>();
	}
	// Enter Regular Crouch State
	else if (Owner().bWantsToCrouch && Owner().CheckCanExitVariableCrouch()) {
		Owner().ResizeCharacterHeight(0.15f, Owner().CrouchedHalfHeight);
		return hsm::SiblingTransition<Crouch>();
	}
	// Enter Walk State
	else if (!Owner().bWantsToCrouch && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}

void PlayerMovementStates::VariableCrouch::Update() {
	float OutCeilingDist = 0.0f;
	if (Owner().CheckNeedsVariableCrouch(OutCeilingDist)) {
		Owner().ResizeCharacterHeight(0.15f, OutCeilingDist / 2.0f);		// Halve the Distance since we are resizing the half-height.
	}

}

hsm::Transition PlayerMovementStates::Sprint::GetTransition() {
	// Enter Walk State
	if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}