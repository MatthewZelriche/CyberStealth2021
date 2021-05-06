// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "hsm.h"

class UStealthPlayerMovement;


/**
 *
 */
struct CYBERSTEALTH2021_API PlayerMovementStates {
	struct Walk : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Walk)

		virtual hsm::Transition GetTransition() override;
	};

	struct Sprint : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Sprint)

		virtual hsm::Transition GetTransition() override;
	};

	struct Crouch : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Crouch)

		virtual hsm::Transition GetTransition() override;
	};

	struct VariableCrouch : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(VariableCrouch)

	};

	struct Slide : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Slide)

	};

	struct Climb : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Climb)

	};
};