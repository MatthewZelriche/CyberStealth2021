#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"

hsm::Transition PlayerMovementStates::Walk::GetTransition() {
	if (Owner().bWantsToCrouch && Owner().CharacterOwner->CanCrouch() && !Owner().PlayerRef->bIsCrouched) {
		return hsm::SiblingTransition<Crouch>();
	}
	else if (Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Sprint>();
	}

	return hsm::NoTransition();
}

hsm::Transition PlayerMovementStates::Crouch::GetTransition() {
	if (Owner().PBCharacter->IsSprinting()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().PlayerRef->UnCrouch();
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Sprint>();
	}
	else if (!Owner().bWantsToCrouch && Owner().CanUncrouch()) {
		Owner().PBCharacter->bIsCrouched = false;
		Owner().ResizeCharacterHeight(Owner().UncrouchTime, Owner().PlayerRef->StandingHeight);
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}

void PlayerMovementStates::Crouch::OnEnter() {
	Owner().PBCharacter->bIsCrouched = true;
	Owner().ResizeCharacterHeight(Owner().CrouchTime, Owner().CrouchedHalfHeight);
}

hsm::Transition PlayerMovementStates::Sprint::GetTransition() {
	if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}