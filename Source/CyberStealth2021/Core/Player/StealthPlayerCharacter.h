// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "StealthPlayerCharacter.generated.h"

class UCameraComponent;
class UStealthPlayerMovement;
/**
 * 
 */
UCLASS()
class CYBERSTEALTH2021_API AStealthPlayerCharacter : public APBPlayerCharacter
{
	GENERATED_BODY()
private:

	UPROPERTY(EditAnywhere)
	UCameraComponent* PlayerCamera;
	UStealthPlayerMovement* StealthMovementPtr;

public:
	AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void Sprint();
	UFUNCTION(BlueprintCallable)
	void StopSprinting();

	virtual void Crouch(bool bClientSimulation) override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraComponent* GetPlayerCamera() { return PlayerCamera; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UStealthPlayerMovement* GetStealthMovementComp() { return StealthMovementPtr; }

	float StandingHeight = 68.0f;
	float StandingEyeHeight = 50.0f;

	// Player Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bToggleCrouch = true;
};
