// Copyright 2017-2019 Project Borealis

#include "Character/PBPlayerMovement.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundCue.h"

#include "Sound/PBMoveStepSound.h"
#include "Character/PBPlayerCharacter.h"

static TAutoConsoleVariable<int32> CVarShowPos(TEXT("cl.ShowPos"), 0, TEXT("Show position and movement information.\n"), ECVF_Default);

DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysFalling"), STAT_CharPhysFalling, STATGROUP_Character);

// MAGIC NUMBERS
const float MAX_STEP_SIDE_Z = 0.08f; // maximum z value for the normal on the vertical side of steps
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle
											  // normals slightly off horizontal for vertical surface.

// Purpose: override default player movement
UPBPlayerMovement::UPBPlayerMovement()
{
	// We have our own air movement handling, so we can allow for full air
	// control through UE's logic
	AirControl = 1.0f;
	// Disable air control boost
	AirControlBoostMultiplier = 1.0f;
	AirControlBoostVelocityThreshold = 0.0f;
	// HL2 cl_(forward & side)speed = 450Hu
	MaxAcceleration = 857.25f;
	// Set the default walk speed
	MaxWalkSpeed = 361.9f;
	WalkSpeed = 285.75f;
	RunSpeed = 361.9f;
	SprintSpeed = 609.6f;
	// Acceleration multipliers (HL2's sv_accelerate and sv_airaccelerate)
	GroundAccelerationMultiplier = 10.0f;
	AirAccelerationMultiplier = 10.0f;
	// 30 air speed cap from HL2
	AirSpeedCap = 57.15f;
	// HL2 like friction
	// sv_friction
	GroundFriction = 4.0f;
	BrakingFriction = 4.0f;
	bUseSeparateBrakingFriction = false;
	// No multiplier
	BrakingFrictionFactor = 1.0f;
	// Braking deceleration (sv_stopspeed)
	FallingLateralFriction = 0.0f;
	BrakingDecelerationFalling = 0.0f;
	BrakingDecelerationFlying = 190.5f;
	BrakingDecelerationSwimming = 190.5f;
	BrakingDecelerationWalking = 190.5f;
	// HL2 step height
	MaxStepHeight = 34.29f;
	// Step height scaling due to speed
	MinStepHeight = 7.5f;
	// Jump z from HL2's 160Hu
	// 21Hu jump height
	// 510ms jump time
	JumpZVelocity = 304.8f;
	// Always have the same jump
	JumpOffJumpZFactor = 1.0f;
	// Default show pos to false
	bShowPos = false;
	// Speed multiplier bounds
	SpeedMultMin = SprintSpeed * 1.7f;
	SpeedMultMax = SprintSpeed * 2.5f;
	// Start out braking
	bBrakingFrameTolerated = true;
	// Crouching
	CrouchedHalfHeight = 34.29f;
	MaxWalkSpeedCrouched = 120.65f;
	bCanWalkOffLedgesWhenCrouching = true;
	CrouchTime = MOVEMENT_DEFAULT_CROUCHTIME;
	UncrouchTime = MOVEMENT_DEFAULT_UNCROUCHTIME;
	CrouchJumpTime = MOVEMENT_DEFAULT_CROUCHJUMPTIME;
	UncrouchJumpTime = MOVEMENT_DEFAULT_UNCROUCHJUMPTIME;
	// Noclip
	NoClipVerticalMoveMode = 0;
	// Slope angle is 45.57 degrees
	SetWalkableFloorZ(0.7f);
	// Tune physics interactions
	StandingDownwardForceScale = 1.0f;
	// Reasonable values polled from NASA (https://msis.jsc.nasa.gov/sections/section04.htm#Figure%204.9.3-6)
	// and Standard Handbook of Machine Design
	InitialPushForceFactor = 100.0f;
	PushForceFactor = 500.0f;
	// Let's not do any weird stuff...Gordon isn't a trampoline
	RepulsionForce = 0.0f;
	MaxTouchForce = 0.0f;
	TouchForceFactor = 0.0f;
	// Just push all objects based on their impact point
	// it might be weird with a lot of dev objects due to scale, but
	// it's much more realistic.
	bPushForceUsingZOffset = false;
	PushForcePointZOffsetFactor = -0.66f;
	// Scale push force down if we are slow
	bScalePushForceToVelocity = true;
	// Don't push more if there's more mass
	bPushForceScaledToMass = false;
	bTouchForceScaledToMass = false;
	Mass = 65.77f; // Gordon is 145lbs
	// Don't smooth rotation at all
	bUseControllerDesiredRotation = false;
	// Flat base
	bUseFlatBaseForFloorChecks = true;
	// Agent props
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanFly = true;
	PBCharacter = Cast<APBPlayerCharacter>(GetOwner());
}

void UPBPlayerMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	bAppliedFriction = false;

	// TODO(mastercoms): HACK: double friction in order to account for insufficient braking on substepping
	if (DeltaTime > MaxSimulationTimeStep)
	{
		BrakingFrictionFactor = 2.0f;
	}
	else
	{
		BrakingFrictionFactor = 1.0f;
	}
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Skip player movement when we're simulating physics (ie ragdoll)
	if (UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	if ((bShowPos || CVarShowPos->GetInt() != 0) && CharacterOwner)
	{
		GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::Green,
										 FString::Printf(TEXT("pos: %f %f %f"), CharacterOwner->GetActorLocation().X, CharacterOwner->GetActorLocation().Y,
														 CharacterOwner->GetActorLocation().Z));
		GEngine->AddOnScreenDebugMessage(2, 1.0f, FColor::Green,
										 FString::Printf(TEXT("ang: %f %f %f"), CharacterOwner->GetControlRotation().Yaw,
														 CharacterOwner->GetControlRotation().Pitch, CharacterOwner->GetControlRotation().Roll));
		GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Green, FString::Printf(TEXT("vel: %f"), FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y)));
	}

	bBrakingFrameTolerated = IsMovingOnGround();
}

bool UPBPlayerMovement::DoJump(bool bClientSimulation)
{
	return bCheatFlying || Super::DoJump(bClientSimulation);
}

void UPBPlayerMovement::TwoWallAdjust(FVector& OutDelta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
	Super::Super::TwoWallAdjust(OutDelta, Hit, OldHitNormal);
}

float UPBPlayerMovement::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	return Super::Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}

FVector UPBPlayerMovement::HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	return SlideResult;
}

bool UPBPlayerMovement::ShouldCatchAir(const FFindFloorResult& OldFloor, const FFindFloorResult& NewFloor)
{
	float SurfaceFriction = 1.0f;
	if (OldFloor.HitResult.PhysMaterial.IsValid())
	{
		UPhysicalMaterial* PhysMat = OldFloor.HitResult.PhysMaterial.Get();
		if (PhysMat)
		{
			SurfaceFriction = FMath::Min(1.0f, PhysMat->Friction * 1.25f);
		}
	}

	float Speed = Velocity.Size2D();
	float MaxSpeed = SprintSpeed * 1.5f;

	float SpeedMult = MaxSpeed / Speed;

	float ZDiff = NewFloor.HitResult.ImpactNormal.Z - OldFloor.HitResult.ImpactNormal.Z;

	if (ZDiff > 0.0f && SurfaceFriction * SpeedMult < 0.5f)
	{
		return true;
	}

	return Super::ShouldCatchAir(OldFloor, NewFloor);
}

void UPBPlayerMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	// Reset step side if we are changing modes
	StepSide = false;
	
	FHitResult Hit;
	// did we jump or land
	bool bJumped = false;

	if (PreviousMovementMode == EMovementMode::MOVE_Walking && MovementMode == EMovementMode::MOVE_Falling)
	{
		// Hit = UPBUtil::TraceLineFullCharacter(CharacterOwner->GetCapsuleComponent(), GetWorld(), CharacterOwner);
		FCollisionQueryParams TraceParams(FName(TEXT("RV_Trace")), true, CharacterOwner);
		TraceParams.bTraceComplex = CharacterOwner->GetCapsuleComponent()->bTraceComplexOnMove;
		TraceParams.bReturnPhysicalMaterial = true;

		GetWorld()->SweepSingleByChannel(Hit, CharacterOwner->GetCapsuleComponent()->GetComponentLocation(),
									  CharacterOwner->GetCapsuleComponent()->GetComponentLocation() - FVector(0.0f, 0.0f, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f),
									  FQuat::Identity, ECC_Visibility,
									  FCollisionShape::MakeBox(FVector(CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(),
																	   CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.5f)),
									  TraceParams);
		bJumped = true;
	}
	if (PreviousMovementMode == EMovementMode::MOVE_Falling && MovementMode == EMovementMode::MOVE_Walking)
	{
		Hit = CurrentFloor.HitResult;
	}

	UPBMoveStepSound* MoveSound = nullptr;
	TSubclassOf<UPBMoveStepSound>* GotSound = nullptr;
	if (Hit.PhysMaterial.IsValid())
	{
		GotSound = PBCharacter->GetMoveStepSound(Hit.PhysMaterial->SurfaceType);
	}
	if (GotSound)
	{
		MoveSound = GotSound->GetDefaultObject();
	}
	if (!MoveSound)
	{
		if (!PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType_Default)))
		{
			return;
		}
		MoveSound = PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType_Default))->GetDefaultObject();
	}

	if (MoveSound)
	{
		float MoveSoundVolume = MoveSound->GetWalkVolume();

		if (IsCrouching())
		{
			MoveSoundVolume *= 0.65f;
		}

		TArray<USoundCue*> MoveSoundCues = bJumped ? MoveSound->GetJumpSounds() : MoveSound->GetLandSounds();

		if (MoveSoundCues.Num() < 1)
		{
			return;
		}

		USoundCue* Sound = MoveSoundCues[FMath::RandRange(0, MoveSoundCues.Num() - 1)];

		Sound->VolumeMultiplier = MoveSoundVolume;

		/*UPBGameplayStatics::PlaySound(Sound, GetCharacterOwner(),
									  // FVector(0.0f, 0.0f, -GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
									  EPBSoundCategory::Footstep);*/
		UGameplayStatics::SpawnSoundAttached(Sound, GetCharacterOwner()->GetRootComponent());
	}
}

void UPBPlayerMovement::ToggleNoClip()
{
	if (bCheatFlying)
	{
		SetMovementMode(MOVE_Walking);
		bCheatFlying = false;
		GetCharacterOwner()->SetActorEnableCollision(true);
	}
	else
	{
		SetMovementMode(MOVE_Flying);
		bCheatFlying = true;
		GetCharacterOwner()->SetActorEnableCollision(false);
	}
}

void UPBPlayerMovement::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	float Speed = Velocity.Size2D();
	if (Speed <= 0.1f || !HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	const float FrictionFactor = FMath::Max(0.0f, BrakingFrictionFactor);
	Friction = FMath::Max(0.0f, Friction * FrictionFactor);
	BrakingDeceleration = FMath::Max(BrakingDeceleration, Speed);
	BrakingDeceleration = FMath::Max(0.0f, BrakingDeceleration);
	const bool bZeroFriction = FMath::IsNearlyZero(Friction);
	const bool bZeroBraking = BrakingDeceleration == 0.0f;

	if (bZeroFriction || bZeroBraking)
	{
		return;
	}

	const FVector OldVel = Velocity;

	// Decelerate to brake to a stop
	const FVector RevAccel = Friction * BrakingDeceleration * Velocity.GetSafeNormal();
	Velocity -= RevAccel * DeltaTime;

	// Don't reverse direction
	if ((Velocity | OldVel) <= 0.0f)
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	// Clamp to zero if nearly zero, or if below min threshold and braking.
	const float VSizeSq = Velocity.SizeSquared();
	if (VSizeSq <= KINDA_SMALL_NUMBER)
	{
		Velocity = FVector::ZeroVector;
	}
}

void UPBPlayerMovement::PlayMoveSound(float DeltaTime)
{
	// Count move sound time down if we've got it
	if (MoveSoundTime > 0)
	{
		MoveSoundTime = FMath::Max(0.0f, MoveSoundTime - 1000.0f * DeltaTime);
	}

	// Check if it's time to play the sound
	if (MoveSoundTime > 0)
	{
		return;
	}

	float Speed = Velocity.SizeSquared();
	float RunSpeedThreshold;
	float SprintSpeedThreshold;

	if (IsCrouching() || bOnLadder)
	{
		RunSpeedThreshold = MaxWalkSpeedCrouched;
		SprintSpeedThreshold = MaxWalkSpeedCrouched * 1.7f;
	}
	else
	{
		RunSpeedThreshold = MaxWalkSpeed;
		SprintSpeedThreshold = SprintSpeed;
	}

	// Only play sounds if we are moving fast enough on the ground or on a
	// ladder
	bool bPlaySound = (bBrakingFrameTolerated || bOnLadder) && Speed >= RunSpeedThreshold * RunSpeedThreshold;

	if (!bPlaySound)
	{
		return;
	}

	bool bSprinting = Speed >= SprintSpeedThreshold * SprintSpeedThreshold;

	float MoveSoundVolume = 1.0f;

	UPBMoveStepSound* MoveSound = nullptr;

	if (bOnLadder)
	{
		MoveSoundVolume = 0.5f;
		MoveSoundTime = 450.0f;
		if (!PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType1)))
		{
			return;
		}
		MoveSound = PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType1))->GetDefaultObject();
	}
	else
	{
		MoveSoundTime = bSprinting ? 300.0f : 400.0f;
		FHitResult Hit = CurrentFloor.HitResult;
		TSubclassOf<UPBMoveStepSound>* GotSound = nullptr;
		if (Hit.PhysMaterial.IsValid())
		{
			GotSound = PBCharacter->GetMoveStepSound(Hit.PhysMaterial->SurfaceType);
		}
		if (GotSound)
		{
			MoveSound = GotSound->GetDefaultObject();
		}
		if (!MoveSound)
		{
			if (!PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType_Default)))
			{
				return;
			}
			MoveSound = PBCharacter->GetMoveStepSound(TEnumAsByte<EPhysicalSurface>(EPhysicalSurface::SurfaceType_Default))->GetDefaultObject();
		}

		MoveSoundVolume = bSprinting ? MoveSound->GetSprintVolume() : MoveSound->GetWalkVolume();

		if (IsCrouching())
		{
			MoveSoundVolume *= 0.65f;
			MoveSoundTime += 100.0f;
		}
	}

	if (MoveSound)
	{
		TArray<USoundCue*> MoveSoundCues = StepSide ? MoveSound->GetStepLeftSounds() : MoveSound->GetStepRightSounds();

		if (MoveSoundCues.Num() < 1)
		{
			return;
		}

		USoundCue* Sound = MoveSoundCues[FMath::RandRange(0, MoveSoundCues.Num() - 1)];

		Sound->VolumeMultiplier = MoveSoundVolume;

		/*UPBGameplayStatics::PlaySound(Sound, GetCharacterOwner(),
									  // FVector(0.0f, 0.0f, -GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
									  EPBSoundCategory::Footstep);*/
		UGameplayStatics::SpawnSoundAttached(Sound, GetCharacterOwner()->GetRootComponent());
	}

	StepSide = !StepSide;
}

void UPBPlayerMovement::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	PlayMoveSound(DeltaTime);

	// Do not update velocity when using root motion or when SimulatedProxy -
	// SimulatedProxy are repped their Velocity
	if (!HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME ||
		(CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy && !bWasSimulatingRootMotion))
	{
		return;
	}

	Friction = FMath::Max(0.0f, Friction);
	const float MaxAccel = GetMaxAcceleration();
	float MaxSpeed = GetMaxSpeed();

	// Check if path following requested movement
	bool bZeroRequestedAcceleration = true;
	FVector RequestedAcceleration = FVector::ZeroVector;
	float RequestedSpeed = 0.0f;
	if (ApplyRequestedMove(DeltaTime, MaxAccel, MaxSpeed, Friction, BrakingDeceleration, RequestedAcceleration, RequestedSpeed))
	{
		RequestedAcceleration = RequestedAcceleration.GetClampedToMaxSize(MaxAccel);
		bZeroRequestedAcceleration = false;
	}

	if (bForceMaxAccel)
	{
		// Force acceleration at full speed.
		// In consideration order for direction: Acceleration, then Velocity, then Pawn's rotation.
		if (Acceleration.SizeSquared() > SMALL_NUMBER)
		{
			Acceleration = Acceleration.GetSafeNormal() * MaxAccel;
		}
		else
		{
			Acceleration = MaxAccel * (Velocity.SizeSquared() < SMALL_NUMBER ? UpdatedComponent->GetForwardVector() : Velocity.GetSafeNormal());
		}

		AnalogInputModifier = 1.0f;
	}

	// Path following above didn't care about the analog modifier, but we do for everything else below, so get the fully modified value.
	// Use max of requested speed and max speed if we modified the speed in ApplyRequestedMove above.
	const float MaxInputSpeed = FMath::Max(MaxSpeed * AnalogInputModifier, GetMinAnalogSpeed());
	MaxSpeed = FMath::Max(RequestedSpeed, MaxInputSpeed);

	// Apply braking or deceleration
	const bool bZeroAcceleration = Acceleration.IsNearlyZero();
	const bool bIsGroundMove = IsMovingOnGround() && bBrakingFrameTolerated;

	float SurfaceFriction = 1.0f;
	UPhysicalMaterial* PhysMat = CurrentFloor.HitResult.PhysMaterial.Get();
	if (PhysMat)
	{
		SurfaceFriction = FMath::Min(1.0f, PhysMat->Friction * 1.25f);
	}

	// Apply friction
	// TODO: HACK: friction applied only once in substepping due to excessive friction, but this is too little for low frame rates
	if (bIsGroundMove && !bAppliedFriction)
	{
		const float ActualBrakingFriction = (bUseSeparateBrakingFriction ? BrakingFriction : Friction) * SurfaceFriction;
		ApplyVelocityBraking(DeltaTime, ActualBrakingFriction, BrakingDeceleration);
		bAppliedFriction = true;
	}

	// Apply fluid friction
	if (bFluid)
	{
		Velocity = Velocity * (1.0f - FMath::Min(Friction * DeltaTime, 1.0f));
	}

	// no clip
	if (bCheatFlying)
	{
		if (bZeroAcceleration)
		{
			Velocity = FVector(0.0f);
		}
		else
		{
			auto LookVec = CharacterOwner->GetControlRotation().Vector();
			auto LookVec2D = CharacterOwner->GetActorForwardVector();
			LookVec2D.Z = 0.0f;
			auto PerpendicularAccel = (LookVec2D | Acceleration) * LookVec2D;
			auto TangentialAccel = Acceleration - PerpendicularAccel;
			auto UnitAcceleration = Acceleration;
			auto Dir = UnitAcceleration.CosineAngle2D(LookVec);
			auto NoClipAccelClamp = PBCharacter->IsSprinting() ? 2.0f * MaxAcceleration : MaxAcceleration;
			Velocity = (Dir * LookVec * PerpendicularAccel.Size2D() + TangentialAccel).GetClampedToSize(NoClipAccelClamp, NoClipAccelClamp);
		}
	}
	else
	{
		// Apply input acceleration
		if (!bZeroAcceleration)
		{
			// Clamp acceleration to max speed
			Acceleration = Acceleration.GetClampedToMaxSize2D(MaxSpeed);
			// Find veer
			const FVector AccelDir = Acceleration.GetSafeNormal2D();
			const float Veer = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y;
			// Get add speed with air speed cap
			const float AddSpeed = (bIsGroundMove ? Acceleration : Acceleration.GetClampedToMaxSize2D(AirSpeedCap)).Size2D() - Veer;
			if (AddSpeed > 0.0f)
			{
				// Apply acceleration
				float AccelerationMultiplier = bIsGroundMove ? GroundAccelerationMultiplier : AirAccelerationMultiplier;
				Acceleration *= AccelerationMultiplier * SurfaceFriction * DeltaTime;
				Acceleration = Acceleration.GetClampedToMaxSize2D(AddSpeed);
				Velocity += Acceleration;
			}
		}

		// Apply additional requested acceleration
		if (!bZeroRequestedAcceleration)
		{
			Velocity += RequestedAcceleration * DeltaTime;
		}

		Velocity = Velocity.GetClampedToMaxSize2D(13470.4f);

		float SpeedSq = Velocity.SizeSquared2D();

		// Dynamic step height code for allowing sliding on a slope when at a high speed
		if (SpeedSq <= MaxWalkSpeedCrouched * MaxWalkSpeedCrouched)
		{
			// If we're crouching or not sliding, just use max
			MaxStepHeight = GetClass()->GetDefaultObject<UPBPlayerMovement>()->MaxStepHeight;
			SetWalkableFloorZ(GetClass()->GetDefaultObject<UPBPlayerMovement>()->GetWalkableFloorZ());
		}
		else
		{
			// Scale step/ramp height down the faster we go
			float Speed = FMath::Sqrt(SpeedSq);
			float SpeedScale = (Speed - SpeedMultMin) / (SpeedMultMax - SpeedMultMin);
			// float SpeedMultiplier = UPBUtil::Clamp01(SpeedScale);
			float SpeedMultiplier = FMath::Clamp(SpeedScale, 0.0f, 1.0f);
			SpeedMultiplier *= SpeedMultiplier;
			if (!IsFalling())
			{
				// If we're on ground, factor in friction.
				SpeedMultiplier = FMath::Max((1.0f - SurfaceFriction) * SpeedMultiplier, 0.0f);
			}
			MaxStepHeight = FMath::Clamp(GetClass()->GetDefaultObject<UPBPlayerMovement>()->MaxStepHeight * (1.0f - SpeedMultiplier), MinStepHeight,
										 GetClass()->GetDefaultObject<UPBPlayerMovement>()->MaxStepHeight);
			SetWalkableFloorZ(FMath::Clamp(GetClass()->GetDefaultObject<UPBPlayerMovement>()->GetWalkableFloorZ() - (0.5f * (0.4f - SpeedMultiplier)),
										   GetClass()->GetDefaultObject<UPBPlayerMovement>()->GetWalkableFloorZ(), 0.9848f));
		}
	}

	if (bUseRVOAvoidance)
	{
		CalcAvoidanceVelocity(DeltaTime);
	}
}