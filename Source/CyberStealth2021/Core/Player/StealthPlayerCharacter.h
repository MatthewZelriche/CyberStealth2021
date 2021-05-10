// Copyright 2021 MatthewZelriche. Licensed under the MIT License. See the included LICENSE.md file for details.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "StealthPlayerCharacter.generated.h"

class UCameraComponent;
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
public:
	AStealthPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void Sprint();
	UFUNCTION(BlueprintCallable)
	void StopSprinting();

	FORCEINLINE UCameraComponent* GetPlayerCamera() { return PlayerCamera; }

	float StandingHeight = 68.0f;
	float StandingEyeHeight = 50.0f;
};
