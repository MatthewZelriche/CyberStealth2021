#include "PlayerMovementStates.h"

#include "StealthPlayerMovement.h"
#include "StealthPlayerCharacter.h"

hsm::Transition PlayerMovementStates::Walk::GetTransition() {
	if (Owner().bWantsToCrouch && Owner().CharacterOwner->CanCrouch()) {
		return hsm::SiblingTransition<Crouch>();
	}
	else if (Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Sprint>();
	}

	return hsm::NoTransition();
}

hsm::Transition PlayerMovementStates::Crouch::GetTransition() {
	if (Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Sprint>();
	}
	else if (!Owner().bWantsToCrouch) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}

hsm::Transition PlayerMovementStates::Sprint::GetTransition() {
	if (!Owner().PBCharacter->IsSprinting()) {
		return hsm::SiblingTransition<Walk>();
	}

	return hsm::NoTransition();
}