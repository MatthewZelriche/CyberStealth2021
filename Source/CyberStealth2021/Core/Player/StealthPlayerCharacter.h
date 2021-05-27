// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "CameraBob.h"
#include "StealthPlayerCharacter.generated.h"

class UCameraComponent;
class UStealthPlayerMovement;
class USpringArmComponent;
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
	UCameraBob* CameraBobComponent;
	UStealthPlayerMovement* StealthMovementPtr;
	USpringArmComponent* CameraAnchor;

	friend UCameraBob;			// Declare UCameraBob as friend so it can access the private OnPlayerStepped() function.
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

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraComponent* GetPlayerCamera() { return PlayerCamera; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UStealthPlayerMovement* GetStealthMovementComp() { return StealthMovementPtr; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE USpringArmComponent* GetCameraAnchor() { return CameraAnchor; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCameraBob* GetCameraBobber() { return CameraBobComponent; }

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
