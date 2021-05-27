// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "hsm.h"

class UStealthPlayerMovement;


/**
 *
 */
struct CYBERSTEALTH2021_API PlayerMovementStates {
	struct GenericLocomotion : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(GenericLocomotion)

		virtual hsm::Transition GetTransition() override;
	};

	struct Slide : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Slide)

		virtual hsm::Transition GetTransition() override;
		virtual void OnEnter() override;
		virtual void OnExit() override;
		virtual void Update() override;

	private:
		bool CheckIfSlideInterrupted();
	};

	struct Climb : hsm::StateWithOwner<UStealthPlayerMovement> {
		DEFINE_HSM_STATE(Climb)

	};

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
		// Disallow use of the base OnEnter() function, since there is never a reason it should be called for this state.
		// Instead, the custom OnEnter() function with parameters should always be used.
		virtual void OnEnter() override { check(false); }
		void OnEnter(float TransitionSpeed, bool bForceCrouch = false);
		virtual void OnExit() override;
		virtual hsm::Transition GetTransition() override;
	};

	struct VariableCrouch : Crouch {
		DEFINE_HSM_STATE(VariableCrouch)
		void OnEnter(bool bForceCrouch, bool bRegularCrouchSpeed = false);
		virtual hsm::Transition GetTransition() override;
		virtual void Update() override;

		bool mRegularCrouchSpeed = false;
		float EntryHeight = 0.0f;
	};
};
