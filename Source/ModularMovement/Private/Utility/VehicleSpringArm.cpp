//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#include "Utility/VehicleSpringArm.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "DrawDebugHelpers.h"
#include "ModularMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


//////////////////////////////////////////////////////////////////////////
// UVehicleSpringArm

const FName UVehicleSpringArm::SocketName(TEXT("SpringEndpoint"));

UVehicleSpringArm::UVehicleSpringArm(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bDrawDebugLagMarkers(0), CurrentCooldown(0), PreviousSpeed(0), CurrentArmLen(0)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	bAutoActivate = true;
	bTickInEditor = true;
	bUsePawnControlRotation = false;
	bDoCollisionTest = true;

	bInheritPitch = true;
	bInheritYaw = true;
	bInheritRoll = true;

	TargetArmLength = 300.0f;
	ProbeSize = 12.0f;
	ProbeChannel = ECC_Camera;

	RelativeSocketRotation = FQuat::Identity;

	bUseCameraLagSubstepping = true;
	bEnableCameraLag = true;
	bEnableCameraRotationLag = true;
	CameraLagZSpeed = 5;
	CameraRotationLagSpeed = 2.f;
	CameraLagMaxDistance = 100.f;
	bUsePawnControlRotation = true;
	TargetArmLength = 700.f;
	CameraLagMaxTimeStep = 1.f / 60.f;
	CameraLagMaxDistance = 0.f;

	UnfixedCameraPosition = FVector::ZeroVector;
}

FRotator UVehicleSpringArm::GetDesiredRotation() const
{
	return GetComponentRotation();
}

FRotator UVehicleSpringArm::GetTargetRotation() const
{
	FRotator DesiredRot = GetDesiredRotation();

	if (bUsePawnControlRotation)
	{
		if (const APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			const FRotator PawnViewRotation = OwningPawn->GetViewRotation();
			if (DesiredRot != PawnViewRotation)
			{
				DesiredRot = PawnViewRotation;
			}
		}
	}

	// If inheriting rotation, check options for which components to inherit
	if (!IsUsingAbsoluteRotation())
	{
		const FRotator LocalRelativeRotation = GetRelativeRotation();
		if (!bInheritPitch)
		{
			DesiredRot.Pitch = LocalRelativeRotation.Pitch;
		}

		if (!bInheritYaw)
		{
			DesiredRot.Yaw = LocalRelativeRotation.Yaw;
		}

		if (!bInheritRoll)
		{
			DesiredRot.Roll = LocalRelativeRotation.Roll;
		}
	}

	return DesiredRot;
}

void UVehicleSpringArm::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag,
                                                 float DeltaTime)
{
	FRotator DesiredRot = GetTargetRotation();

	// Apply 'lag' to rotation if desired
	if (bDoRotationLag && CurrentCooldown <= 0.f)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRot - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount,
				                                       CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRot;
			}
		}
		else
		{
			DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRot), DeltaTime,
			                                       CameraRotationLagSpeed));
		}
	}
	PreviousDesiredRot = DesiredRot;

	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;
	if (bDoLocationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagZSpeed > 0.f)
		{
			const FVector ArmMovementStep = (DesiredLoc - PreviousDesiredLoc) * (1.f / DeltaTime);
			FVector LerpTarget = PreviousDesiredLoc;

			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmMovementStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagZSpeed);
				PreviousDesiredLoc = DesiredLoc;
			}
		}
		else
		{
			DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, DesiredLoc, DeltaTime, CameraLagZSpeed);
		}

		// Clamp distance if requested
		bool bClampedDist = false;
		if (CameraLagMaxDistance > 0.f)
		{
			const FVector FromOrigin = DesiredLoc - ArmOrigin;
			if (FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
			{
				DesiredLoc = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
				bClampedDist = true;
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugLagMarkers)
		{
			DrawDebugSphere(GetWorld(), ArmOrigin, 5.f, 8, FColor::Green);
			DrawDebugSphere(GetWorld(), DesiredLoc, 5.f, 8, FColor::Yellow);

			const FVector ToOrigin = ArmOrigin - DesiredLoc;
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc, DesiredLoc + ToOrigin * 0.5f, 7.5f,
			                          bClampedDist ? FColor::Red : FColor::Green);
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc + ToOrigin * 0.5f, ArmOrigin, 7.5f,
			                          bClampedDist ? FColor::Red : FColor::Green);
		}
#endif
	}
	DesiredLoc.X = ArmOrigin.X;
	DesiredLoc.Y = ArmOrigin.Y;

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * CurrentArmLen;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (CurrentArmLen != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel,
		                                 FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}

FVector UVehicleSpringArm::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation,
                                          bool bHitSomething, float DeltaTime)
{
	return bHitSomething ? TraceHitLocation : DesiredArmLocation;
}

void UVehicleSpringArm::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);
	PreviousDesiredLoc += InOffset;
	PreviousArmOrigin += InOffset;
}

void UVehicleSpringArm::OnRegister()
{
	Super::OnRegister();

	// enforce reasonable limits to avoid potential div-by-zero
	CameraLagMaxTimeStep = FMath::Max(CameraLagMaxTimeStep, 1.f / 200.f);
	CameraLagZSpeed = FMath::Max(CameraLagZSpeed, 0.f);

	CurrentArmLen = TargetArmLength;
	// Set initial location (without lag).
	UpdateDesiredArmLocation(false, false, false, 0.f);
}

void UVehicleSpringArm::PostLoad()
{
	Super::PostLoad();
}

void UVehicleSpringArm::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (const APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		if (const UModularMovementComponent* MC = Cast<UModularMovementComponent>(OwningPawn->GetMovementComponent()))
		{
			if (MC->GetNumberOfDriveWheelsTouchingGround() == 0)
			{
				AirborneTime += DeltaTime;
			}
			else
			{
				if (AirborneTime > MinAirborneTimeForCameraShake)
				{
					//play shake

					if (OwningPawn->GetController() && OwningPawn->GetController()->IsPlayerController())
					{
						APlayerController* PC = Cast<APlayerController>(OwningPawn->GetController());
						PC->ClientStartCameraShake(CameraShake);
					}
				}
				AirborneTime = 0.f;
			}
		}
		const FRotator CurrentRot = bUsePawnControlRotation ? OwningPawn->GetViewRotation() : GetComponentRotation();
		if (UMeshComponent* Mesh = Cast<UMeshComponent>(GetOwner()->GetRootComponent()))
		{
			const FVector Velocity = Mesh->GetPhysicsLinearVelocity() * FVector(1, 1, 0);

			if (CurrentCooldown > 0.f)
			{
				CurrentCooldown -= DeltaTime;
			}
			else
			{
				//orient camera towards the speed vector


				if (Velocity.Size() > AutoCorrectMinSpeedRange)
				{
					const float Interpolation = UKismetMathLibrary::MapRangeClamped(
							Velocity.Size(), AutoCorrectMinSpeedRange, AutoCorrectMaxSpeedRange, 0, 1) *
						AutoCorrectInterpolationStrength;
					FRotator Result = UKismetMathLibrary::RInterpTo(CurrentRot, Velocity.Rotation(), DeltaTime,
					                                                Interpolation);
					if (IgnorePitch)
					{
						Result.Pitch = CurrentRot.Pitch;
					}
					if (bUsePawnControlRotation)
					{
						if (OwningPawn->GetController())
						{
							if (OwningPawn->GetController()->IsLocalController())
							{
								OwningPawn->GetController()->SetControlRotation(Result);
							}
						}
					}
					else
					{
						SetWorldRotation(Result);
					}
				}
				//Find lag amount by acceleration
				if (bEnableCameraLag)
				{
					const float Acceleration = ((Velocity * FVector(1, 1, 0)).Size() - PreviousSpeed) / DeltaTime;
					const float InterpolationTarget = UKismetMathLibrary::MapRangeClamped(
						Acceleration, MinAcceleration, MaxAcceleration, TargetArmLength - MaxArmLenChange,
						TargetArmLength + MaxArmLenChange);
					CurrentArmLen = UKismetMathLibrary::FInterpTo(CurrentArmLen, InterpolationTarget, DeltaTime,
					                                              ArmLenInterpolationSpeed);
				}
			}

			PreviousSpeed = (Velocity * FVector(1, 1, 0)).Size();
		}
	}


	UpdateDesiredArmLocation(bDoCollisionTest, bEnableCameraLag, bEnableCameraRotationLag, DeltaTime);
}

FTransform UVehicleSpringArm::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	const FTransform RelativeTransform(RelativeSocketRotation, RelativeSocketLocation);

	switch (TransformSpace)
	{
	case RTS_World:
		{
			return RelativeTransform * GetComponentTransform();
			
		}
	case RTS_Actor:
		{
			if (const AActor* Actor = GetOwner())
			{
				const FTransform SocketTransform = RelativeTransform * GetComponentTransform();
				return SocketTransform.GetRelativeTransform(Actor->GetTransform());
			}
			break;
		}
	case RTS_Component:
		{
			return RelativeTransform;
		}
	default:
		{
			return RelativeTransform;
		}
	}
	return RelativeTransform;
}

bool UVehicleSpringArm::HasAnySockets() const
{
	return true;
}

void UVehicleSpringArm::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	new(OutSockets) FComponentSocketDescription(SocketName, EComponentSocketType::Socket);
}

FVector UVehicleSpringArm::GetUnfixedCameraPosition() const
{
	return UnfixedCameraPosition;
}

bool UVehicleSpringArm::IsCollisionFixApplied() const
{
	return bIsCameraFixed;
}

void UVehicleSpringArm::SetCooldown(float In)
{
	CurrentCooldown = In;
}
