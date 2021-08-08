// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "CameraFXHandler.h"
#include "StealthPlayerCharacter.generated.h"

class UCameraComponent;
class UStealthPlayerMovement;
class USpringArmComponent;
class APlayerCameraManager;
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
	UPROPERTY(EditAnywhere)
	UCameraFXHandler* CameraFXHandler;
	UStealthPlayerMovement* StealthMovementPtr;
	USpringArmComponent* CameraAnchor;

	APlayerCameraManager* cameraManager;

	bool bIsAvailableForLedgeGrab = false;
	float LastJumpLiftoffZPos = 0.0f;

	friend UCameraFXHandler;			// Declare UCameraBob as friend so it can access the private OnPlayerStepped() function.
protected:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPlayerStepped();

public:
	AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void Sprint();
	UFUNCTION(BlueprintCallable)
	void StopSprinting();
	UFUNCTION(BlueprintCallable)
	void LookY(float value);
	UFUNCTION(BlueprintCallable)
	void LookX(float value);

	virtual void Crouch(bool bClientSimulation) override;
	virtual void OnJumped_Implementation() override;
	virtual void NotifyJumpApex() override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraComponent* GetPlayerCamera() { return PlayerCamera; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UStealthPlayerMovement* GetStealthMovementComp() { return StealthMovementPtr; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE USpringArmComponent* GetCameraAnchor() { return CameraAnchor; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraFXHandler* GetCameraFXHandler() { return CameraFXHandler; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsAvailableForLedgeGrab() { return bIsAvailableForLedgeGrab; }

	// Gets the Z level the player was at when they first started the most recent jump.
	// Currently used for calculate how fast a climb should be in StealthPlayerMovement
	FORCEINLINE float GetLastJumpStartingZPos() { return LastJumpLiftoffZPos; }
	FORCEINLINE APlayerCameraManager* GetCameraManager() { return cameraManager; }

	float StandingHeight = 68.0f;
	float StandingEyeHeight = 50.0f;

	// Should crouch be toggled, or "hold-to-crouch"?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bToggleCrouch = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float XMouseSensitivity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float YMouseSensitivity = 1.0f;
};
