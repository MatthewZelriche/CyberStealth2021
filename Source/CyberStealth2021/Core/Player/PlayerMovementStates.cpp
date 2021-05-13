#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"

hsm::Transition PlayerMovementStates::Walk::GetTransition() {
	if (Owner().bWantsToCrouch && Owner().CharacterOwner->CanCrouch() && !Owner().PlayerRef->bIsCrouched) {
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