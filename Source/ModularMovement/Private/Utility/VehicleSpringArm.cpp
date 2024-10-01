//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#include "Utility/VehicleSpringArm.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"

#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "DrawDebugHelpers.h"
#include "ModularMovementComponent.h"
#include "Camera/CameraComponent.h"
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

FRotator UVehicleSpringArm::SafeRotatorInterpolation(const FRotator& From, const FRotator& To, float Alpha, float Speed)
{
	if (From.ContainsNaN() || To.ContainsNaN())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid rotation detected in UVehicleSpringArm::SafeRotatorInterpolation"));
		return FRotator::ZeroRotator;
	}

	FQuat FromQuat = FQuat(From);
	FQuat ToQuat = FQuat(To);
    
	if (!FromQuat.IsNormalized() || !ToQuat.IsNormalized())
	{
		UE_LOG(LogTemp, Error, TEXT("Non-normalized quaternion detected in UVehicleSpringArm::SafeRotatorInterpolation"));
		return FRotator::ZeroRotator;
	}

	return FRotator(FMath::QInterpTo(FromQuat, ToQuat, Alpha, Speed));
}


void UVehicleSpringArm::SetCameraLock(bool InCameraLock)
{
	if(InCameraLock!=bLockToOrientation)
	{
		

		if(InCameraLock)
		{
			Denominator=GetOwner()->GetActorRotation().Yaw;
		}else
		{
			if (const APawn* OwningPawn = Cast<APawn>(GetOwner()))
			{
				if(OwningPawn->Controller)
				{
					OwningPawn->Controller->SetControlRotation(GetTargetRotation());
				}
			}
			
			
		}

		bLockToOrientation=InCameraLock;
	}
}

void UVehicleSpringArm::ToggleCameraLock()
{
	SetCameraLock(!bLockToOrientation);
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
	if(bLockToOrientation)
	{
		DesiredRot.Yaw=DesiredRot.Yaw+GetOwner()->GetActorRotation().Yaw-Denominator;
	}

	
	return DesiredRot;
}

void UVehicleSpringArm::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag,
                                                 float DeltaTime)
{
	FRotator DesiredRot = GetTargetRotation();

	// Calculate and apply camera shake
	FRotator OwnerRot = GetOwner()->GetActorRotation();
    
	// Calculate angular velocity
	FRotator DeltaRot =UKismetMathLibrary::NormalizedDeltaRotator(OwnerRot,LastOwnerRot);		
	FVector DeltaEuler = DeltaRot.Euler();
	AngularVelocity = DeltaEuler / DeltaTime;

	
	SmoothedAngularVelocity = FMath::VInterpTo(SmoothedAngularVelocity, AngularVelocity, DeltaTime, 1.0f / SmoothingFactor);

	if (SmoothedAngularVelocity.ContainsNaN()) {
		SmoothedAngularVelocity = FVector::ZeroVector;
	}
	// Apply shake based on smoothed angular velocity
	
	const FRotator Shake(SmoothedAngularVelocity.Y * AngularVelocityShakeMultiplier,0.f,-SmoothedAngularVelocity.X * AngularVelocityShakeMultiplier);

	// Apply shake to desired rotation
	

	LastOwnerRot = OwnerRot;


	
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

				DesiredRot = SafeRotatorInterpolation(PreviousDesiredRot, LerpTarget, LerpAmount, CameraRotationLagSpeed);
				DesiredRot += Shake;
				PreviousDesiredRot = DesiredRot;
			}
		}
		else
		{
			DesiredRot = SafeRotatorInterpolation(PreviousDesiredRot, DesiredRot, DeltaTime, CameraRotationLagSpeed);
			DesiredRot += Shake;
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
	if(!RelCamTM.ContainsNaN())
	{
		RelativeSocketLocation = RelCamTM.GetLocation();
		RelativeSocketRotation = RelCamTM.GetRotation();
	}
	RelativeSocketRotation.Normalize();
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

	
	FRotator Result;
	if (const APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{

		float DotProduct=LastAutoRot.Vector().Dot( OwningPawn->GetControlRotation().Vector());
		if(DotProduct<0)
		{
			DotProduct=1+DotProduct;
		}
		if(DotProduct<PauseSensitivity)
		{
			SetCooldown(PauseSeconds);
		}
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
		 FRotator CurrentRot = bUsePawnControlRotation ? OwningPawn->GetViewRotation() : GetComponentRotation();
		if(MinPitch!=0.f)
		{
			
			if(CurrentRot.Pitch<180.f)
			{
				CurrentRot.Pitch=FMath::Min(MinPitch,CurrentRot.Pitch);
			}else
			{
				CurrentRot.Pitch=FMath::Min(360+MinPitch,CurrentRot.Pitch);
				
			}
			if(bUsePawnControlRotation)
			{
				if(OwningPawn->GetController())
				{
					OwningPawn->GetController()->SetControlRotation(CurrentRot);
				}
			}
			
		
		}
		if (UMeshComponent* Mesh = Cast<UMeshComponent>(GetOwner()->GetRootComponent()))
		{
			const FVector Velocity = Mesh->GetPhysicsLinearVelocity() * FVector(1, 1, 0);

			if (CurrentCooldown > 0.f)
			{
				CurrentCooldown -= DeltaTime;
			}
			else
			{
				if(AutoCorrect&&!bLockToOrientation)
				{
					//orient camera towards the speed vector


					if (Velocity.Size() > AutoCorrectMinSpeedRange)
					{
						const float Interpolation = UKismetMathLibrary::MapRangeClamped(
								Velocity.Size(), AutoCorrectMinSpeedRange, AutoCorrectMaxSpeedRange, 0, 1) *
							AutoCorrectInterpolationStrength;
						 Result = UKismetMathLibrary::RInterpTo(CurrentRot, Velocity.Rotation(), DeltaTime,
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
						LastAutoRot=Result;
					}else
					{
						LastAutoRot= OwningPawn->GetControlRotation();
					}
					
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
				
			}else
			{
				CurrentArmLen=TargetArmLength;
			}
			PreviousSpeed = (Velocity * FVector(1, 1, 0)).Size();
		}

		
		
		if(CurrentCooldown>0)
		{
			LastAutoRot= OwningPawn->GetControlRotation();
		}


		//Shake loop

	
		
	}



	
	
	
	UpdateDesiredArmLocation(bDoCollisionTest, bEnableCameraLag, bEnableCameraRotationLag, DeltaTime);

	if(ChromaticAberration)
	{
		if( UCameraComponent* Camera=Cast<UCameraComponent>( GetChildComponent(0)))
		{
			Camera->PostProcessSettings.ChromaticAberrationStartOffset=0.5;
			Camera->PostProcessSettings.SceneFringeIntensity=UKismetMathLibrary::MapRangeClamped(PreviousSpeed,ChromaticAberrationMinSpeed,ChromaticAberrationMaxSpeed,0,MaxAberration);

			Camera->PostProcessSettings.bOverride_SceneFringeIntensity=true;
		}
	}


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
