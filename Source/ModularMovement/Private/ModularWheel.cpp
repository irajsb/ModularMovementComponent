// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularMovement.h"
#include "WidgetComponent.h"
#include "VehicleDebugWidget.h"
#include "Kismet/KismetMathLibrary.h"


DECLARE_CYCLE_STAT(TEXT("Modular Updage Suspension"), STAT_ModularSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Forces"), STAT_ModularForces, STATGROUP_MovementPhysics);

// Sets default values for this component's properties
UModularWheel::UModularWheel()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UModularWheel::BeginPlay()
{
	Super::BeginPlay();
	
}

void UModularWheel::SetupWheels(UModularMovementComponent* ModularMovementComponent)
{
	//Initialize data
	const FTransform Transform = GetRelativeTransform();
	WheelState.InitialLocalLocation = Transform.GetLocation();
	WheelState.InitialLocalRotation = Transform.GetRotation().Rotator();
	const float SideAngle = WheelState.InitialLocalLocation.Y < 0 ? 1 : -1;
	WheelState.SuspAngle = UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(this) * SideAngle;
}

void UModularWheel::UpdateSuspension(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	if (!ModularMovementComponent)
	{
		return;
	}
	MODULAR_CYCLE_COUNTER(STAT_ModularSuspension)
	
	//Gather data for trace
	WheelState.WheelLoad = FVector::ZeroVector;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	const FTransform MeshTransform = ModularMovementComponent->GetMesh()->GetComponentTransform();
	const FVector ComponentLocation = MeshTransform.TransformPosition(WheelState.InitialLocalLocation + WheelState.WheelSetup->TraceStartOffset);


	const FVector DirectionVector = ModularMovementComponent->GetMesh()->GetUpVector();
	const FVector TraceEnd = ComponentLocation + (DirectionVector * -1 * WheelState.WheelSetup->SuspensionLength);
	FHitResult TraceResult;
	TraceResult.TraceStart = ComponentLocation;
	TraceResult.TraceEnd = TraceEnd;
	TraceResult.bBlockingHit = false;

	TArray<FHitResult> Hits;
	bool ValidHitFound = false;

//Start Trace
	UKismetSystemLibrary::SphereTraceMulti(GetWorld(), ComponentLocation, TraceEnd, WheelState.WheelSetup->WheelRadius,
	                                       ModularMovementComponent->GetSetup()->GetSuspensionTraceTypeQuery(), true,
	                                       ActorsToIgnore, Debug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
	                                       Hits, true);
//Look for valid hits 
	for (auto Hit : Hits)
	{
		if (Hit.bBlockingHit)
		{
			const FVector Position = MeshTransform.InverseTransformPosition(Hit.ImpactPoint) - WheelState.
				InitialLocalLocation;
			if (FMath::Abs(Position.Y) < WheelState.WheelSetup->WheelWidth)
			{
				ValidHitFound = true;
				TraceResult = Hit;

				break;
			}
		}
	}
	if (!ValidHitFound)
	{
	}

//Calculate suspension force and damping 
	const float CurrentLen = FMath::Max<float>(0, TraceResult.Time);
	const float Stiffness = WheelState.WheelSetup->SpringRate * (1 - CurrentLen) * WheelState.WheelSetup->
		SuspensionLength / 100;


	const float SuspensionDiff = (CurrentLen - WheelState.PreviousLen) * WheelState.WheelSetup->SuspensionLength / 100;

	if (CurrentLen - WheelState.PreviousLen < 0)
	{
		WheelState.DampingForce = -1 * (((SuspensionDiff) * GetWheelSetup()->DampingCompress)) / DeltaTime;
	}
	else
	{
		WheelState.DampingForce = -1 * (((SuspensionDiff) * GetWheelSetup()->DampingRebound)) / DeltaTime;
	}

	if (TraceResult.bBlockingHit && ModularMovementComponent->ShouldProcessPhysics())
	{
		const float AngleCorrection = (FVector::DotProduct(TraceResult.ImpactNormal,
		                                                   (TraceResult.TraceStart - TraceResult.TraceEnd).
		                                                   GetUnsafeNormal()));
		WheelState.WheelLoad = ((AngleCorrection * FVector::UpVector * (Stiffness + WheelState.DampingForce)));
		ModularMovementComponent->GetMesh()->GetBodyInstance()->AddForceAtPosition(
			SIForceToUnrealForce(WheelState.WheelLoad), TraceResult.TraceStart, true);
	}


	WheelState.PreviousLen = CurrentLen;
	WheelState.HitResult = TraceResult;
}

void UModularWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularForces)
	if (!ModularMovementComponent)
	{
		return;
	}

	//Get Speed relative to tire space
	const FTransform WorldTransform = ModularMovementComponent->GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	const FVector WorldMeshVelocity = ModularMovementComponent->GetMesh()->GetBodyInstance()->
	                                                            GetUnrealWorldVelocityAtPoint(
		                                                            WheelState.HitResult.TraceStart);
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	//Slip angle 
	WheelState.SlipAngle = FMath::Atan2(GroundVelocityVector.Y, GroundVelocityVector.X);
	WheelState.WheelStatus = Normal;
	FVector FinalForceVector = FVector::ZeroVector;

	//TODO: Add engine braking
	const float RollingResistance = -1 * ModularMovementComponent->RollingResistanceConstant * GroundVelocityVector.X /
		100.0f; //CM/S to M/s 

	WheelState.DriveTorque = WheelState.DriveTorque / (WheelState.WheelSetup->WheelRadius / 100);

	const bool Braking = FMath::Abs(WheelState.DriveTorque) < FMath::Abs(WheelState.BrakeTorque);


	FinalForceVector.X = WheelState.DriveTorque + RollingResistance;

	//TODO more accurate weight distro 
	const float MassPerWheel = (ModularMovementComponent->GetMesh()->GetMass() / ModularMovementComponent->GetNumberOfWheels());
	const float MaxFriction = MassPerWheel * WheelState.WheelSetup->TireFrictionCoefficient * GWorld->GetGravityZ() / -100.f/*Unreal Force TO SI*/;
	const float SILongitudinalVelocity = GroundVelocityVector.X / 100.0f;

//SlipRatio
	SlipRatio = (WheelState.Omega * WheelState.WheelSetup->WheelRadius / 100.f/*CmToM*/ - SILongitudinalVelocity) /
		FMath::Abs(SILongitudinalVelocity);
	const float ForceRequiredToBringToStop = FMath::Abs(
		MassPerWheel * WheelState.WheelSetup->TireFrictionCoefficient * (GroundVelocityVector.X) / 100 / DeltaTime);

	if (Braking)
	{
		FinalForceVector.X = WheelState.BrakeTorque * -1 * FMath::Sign(GroundVelocityVector.X);
		FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -ForceRequiredToBringToStop, ForceRequiredToBringToStop);
		if (FMath::Abs(FinalForceVector.X) > MaxFriction)
		{
			WheelState.WheelStatus = Locked;
		}
	}


	/*if (!Braking && FMath::Abs(FinalForceVector.X)>MaxFriction)
	{
		UE_LOG(LogTemp,Log,TEXT("Drive force spin %f Max Friction %f"),FinalForceVector.X,MaxFriction)
		FinalForceVector.X=MaxFriction*0.8;
		//Match wheel speed to wheel speed at max rpm at current gear *Arcade method*
		WheelState.WheelStatus=Spinning;
		const float DriveRatio =ModularMovementComponent->GetGearInfo(ModularMovementComponent->VehicleState.CurrentGear).GearRatio*ModularMovementComponent->GetSetup()->GetDifferentialRatio();

		
		const float WheelRPM=ModularMovementComponent->GetSetup()->GetMaxRPM()/DriveRatio;
		WheelState.Omega=(WheelRPM*2*PI)/60;
		
	}*/

	if (WheelState.WheelStatus == Normal)
	{
		if (GetWheelSetup()->ApplyDriveForce)
			UE_LOG(LogTemp, Log, TEXT("Wheel normal %f max friction %f"), FinalForceVector.X, MaxFriction);
		WheelState.Omega = GroundVelocityVector.X / WheelState.WheelSetup->WheelRadius;
	}
	else if (WheelState.WheelStatus == Locked)
	{
		WheelState.Omega = 0;
	}

	//Radian speed is per second * Delta Time to calculate speed in this frame 
	WheelState.AngularPosition += WheelState.Omega * DeltaTime;

	while (WheelState.AngularPosition >= PI * 2.f)
	{
		WheelState.AngularPosition -= PI * 2.f;
	}
	while (WheelState.AngularPosition <= -PI * 2.f)
	{
		WheelState.AngularPosition += PI * 2.f;
	}

	//
	float SlipAngleDegrees = FMath::Abs(FMath::RadiansToDegrees(WheelState.SlipAngle));
	if (SlipAngleDegrees > 90)
	{
		SlipAngleDegrees = 180 - SlipAngleDegrees;
	}
	FinalForceVector.Y = WheelState.WheelSetup->SlipAngle.GetRichCurve()->Eval(SlipAngleDegrees) * WheelState.WheelSetup
		->GraphMultiplier;
	if (FinalForceVector.Y > FMath::Abs(ForceRequiredToBringToStop))
	{
		FinalForceVector.Y = FMath::Abs(ForceRequiredToBringToStop);
	}
	if (GroundVelocityVector.Y > 0.0f)
	{
		FinalForceVector.Y = -FinalForceVector.Y;
	}


	FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -MaxFriction, MaxFriction);
	FinalForceVector = SIForceToUnrealForce(FinalForceVector);


	//Apply Forces to bodies 
	if (ModularMovementComponent->ShouldProcessPhysics())
	{
		FVector FrictionForceLocal = FinalForceVector;
		FrictionForceLocal = SteeringRotator.RotateVector(FrictionForceLocal);
		const FVector GroundZVector = WheelState.HitResult.ImpactNormal;
		const FVector GroundXVector = FVector::CrossProduct(ModularMovementComponent->GetMesh()->GetRightVector(),
		                                                    GroundZVector);
		const FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
		const FMatrix Mat(GroundXVector, GroundYVector, GroundZVector,
		                  ModularMovementComponent->GetMesh()->GetComponentLocation());
		const FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);


		ModularMovementComponent->GetMesh()->GetBodyInstance()->AddForceAtPosition(
			FrictionForceVector, WheelState.HitResult.TraceStart, true);
	}
}

void UModularWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent,
                                   float InNormSteering)
{
	if (WheelState.WheelSetup->SteeringWheel)
	{
		const float AISteerMultiplier = ModularMovementComponent->VehicleState.IsAIVehicle
			                                ? ModularMovementComponent->GetSetup()->GetAIMaxSteerMultiplier()
			                                : 1;
		/*if (FMath::Abs(GWheeledVehicleDebugParams.SteeringOverride) > 0.01f)
		{
		SteeringAngle = PWheel.Setup().WheelState.WheelSetup->SteeringMaxAngle * GWheeledVehicleDebugParams.SteeringOverride;
		}
		else*/
		{
			//

			const float WheelSide = WheelState.InitialLocalLocation.Y;

			float OutSteeringAngle = 0.f;

			switch (ModularMovementComponent->GetSetup()->GetSteerType())
			{
			case AngleRatio:
				{
					const bool OutsideWheel = (InNormSteering * WheelSide) > 0.f;
					OutSteeringAngle = InNormSteering * (OutsideWheel
						                                     ? WheelState.WheelSetup->SteeringMaxAngle *
						                                     AISteerMultiplier
						                                     : WheelState.WheelSetup->SteeringMaxAngle *
						                                     AISteerMultiplier * 0.7/*TODO Setup().AngleRatio*/);
				}
				break;

			case Tank:
				{
					const float LeftTrackInput = InNormSteering;
					const float RightTrackInput = -InNormSteering;
					ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer = 0;
					ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer = 0;
					if (FMath::Abs(ModularMovementComponent->RawThrottleInput) > SMALL_NUMBER)
					{
						ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(
							ModularMovementComponent->RawThrottleInput) + LeftTrackInput;
						ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer = FMath::Abs(
							ModularMovementComponent->RawThrottleInput) + RightTrackInput;
					}
					else
					{
						ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(
							ModularMovementComponent->RawThrottleInput) + LeftTrackInput;
						ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer = FMath::Abs(
							ModularMovementComponent->RawThrottleInput) + RightTrackInput;
					}

					if (WheelSide > 0)
					{
						WheelState.TorqueTransferFactor = ModularMovementComponent->VehicleState.TrackRight.
							TorqueTransfer;
					}
					else
					{
						WheelState.TorqueTransferFactor = ModularMovementComponent->VehicleState.TrackLeft.
							TorqueTransfer;
					}
					OutSteeringAngle = 0;
				}
				break;

			default:
			case SingleAngle:
				{
					OutSteeringAngle = WheelState.WheelSetup->SteeringMaxAngle * InNormSteering * AISteerMultiplier;
				}
				break;
			}


			//
			WheelState.SteerAngle = OutSteeringAngle * WheelState.WheelSetup->SteeringMultiplier;
		}
	}
}

void UModularWheel::SetDriveTorqueOnWheels(float Force)
{
	if (WheelState.WheelSetup->ApplyDriveForce)
	{
		WheelState.DriveTorque = Force;
	}
	else
	{
		WheelState.DriveTorque = 0;
	}
}

float UModularWheel::GetFastestWheelOmegaSpeed()
{
	if (WheelState.WheelSetup->ApplyDriveForce)
	{
		return WheelState.Omega;
	}
	return 0.0f;
}


void UModularWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	if (!AnimateChildComponent)
	{
		return;
	}
	FVector Location;
	FRotator Rotation;

	GetWheelAnimationData(Location, Rotation, DeltaTime);
	TArray<USceneComponent*> Components;
	GetChildrenComponents(false, Components);
	for (USceneComponent* Mesh : Components)
	{
		if (Mesh->IsA(UMeshComponent::StaticClass()))
		{
			if (true)
			{
				//Rotation.Pitch=Rotation.Pitch*-1;
				Rotation = (Rotation.Quaternion().Rotator());
				Mesh->SetRelativeRotation(Rotation);
			}
			else
			{
				SetRelativeRotation(Rotation);
			}


			Mesh->SetRelativeLocation(Location);
		}
	}
}

FTransform UModularWheel::GetWheelTransform()
{
	return GetComponentTransform();
}


void UModularWheel::UpdateWheelState(FWheelState In)
{
	WheelState = In;
}


FWheelState* UModularWheel::GetWheelState()
{
	return &WheelState;
}

UModularVehicleWheelData* UModularWheel::GetWheelSetup() const
{
	return WheelState.WheelSetup;
}

void UModularWheel::UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData)
{
	WheelState.WheelSetup = VehicleWheelData;
}

void UModularWheel::SetSteerOnWheel(float Angle)
{
	WheelState.SteerAngle = Angle;
}


void UModularWheel::GetWheelAnimationData(FVector& Location, FRotator& Rotation, float DeltaTime)
{
	//TODO make sure to execute  this once 


	if (GetWorld()->IsGameWorld() && WheelState.WheelSetup)
	{
		const FVector WheelRadiusVector = FVector(0, 0, WheelState.WheelSetup->WheelRadius);
		const FVector SuspensionTraceLocation = WheelState.HitResult.bBlockingHit
			                                        ? WheelState.HitResult.ImpactPoint + WheelRadiusVector
			                                        : WheelState.HitResult.TraceEnd;


		FTransform WheelTransform = GetWheelTransform();

		//local
		FVector ContactPointPosition = WheelTransform.InverseTransformPosition(SuspensionTraceLocation);


		float Sin, Cos;

		const float ContactPointAngle = FMath::Atan2(ContactPointPosition.X, WheelState.WheelSetup->WheelRadius) * 2;

		FMath::SinCos(&Sin, &Cos, ContactPointAngle);


		FVector ResultPosition = FVector::ZeroVector;
		ResultPosition.Z = ContactPointPosition.Z - ((FMath::Abs(Sin)) * WheelState.WheelSetup->WheelRadius / PI);


		if (WheelState.SuspAngle != 0.0f)
		{
			const float PivotAngle = FMath::Atan2(ResultPosition.Z, WheelState.WheelSetup->SuspensionPivot);
			FMath::SinCos(&Sin, &Cos, PivotAngle);
			ResultPosition.Y = FMath::Abs(Sin) * WheelState.WheelSetup->SuspensionPivot * FMath::Sign(
				WheelState.InitialLocalLocation.Y) * -0.5;
		}

		if (WheelState.WheelSetup->AnimSpeed != 0.0f)
		{
			ResultPosition = UKismetMathLibrary::VInterpTo_Constant(WheelState.PreviousLocation, ResultPosition,
			                                                        DeltaTime, WheelState.WheelSetup->AnimSpeed);
		}


		Location = ResultPosition;
		WheelState.PreviousLocation = ResultPosition;
		const UModularMovementComponent* MovementComponent = Cast<UModularMovementComponent>(
			Cast<APawn>(GetOwner())->GetMovementComponent());
		const float Steer = MovementComponent
			                    ? UKismetMathLibrary::FInterpTo_Constant(
				                    WheelState.PreviousYaw, WheelState.SteerAngle, DeltaTime,
				                    MovementComponent->GetSetup()->GetSteeringAnimationSpeed())
			                    : WheelState.SteerAngle;


		Rotation = FRotator(FMath::RadiansToDegrees(-1 * WheelState.AngularPosition), Steer, 0);

		WheelState.PreviousYaw = Steer;

		if (WheelState.SuspAngle != 0.0f)
		{
			const float CurrentAngle = UKismetMathLibrary::MapRangeClamped(
				ResultPosition.Z, WheelState.WheelSetup->SuspensionLength, -WheelState.WheelSetup->SuspensionLength,
				-WheelState.SuspAngle, WheelState.SuspAngle);
			Rotation = UKismetMathLibrary::ComposeRotators(Rotation, FRotator(0.00f, 0.00f, -CurrentAngle));

			WheelState.CurrentPivotAngle = CurrentAngle;
		}
	}
}

float UModularWheel::GetWheelRotation()
{
	return FMath::RadiansToDegrees(-1 * GetWheelState()->AngularPosition);
}

float UModularWheel::GetWheelPivotRotation()
{
	return GetWheelState()->CurrentPivotAngle;
}

float UModularWheel::GetWheelSteeringValue()
{
	return GetWheelState()->SteerAngle;
}

float UModularWheel::GetWheelCompressionValue()
{
	return 1 - WheelState.HitResult.Time;
}

float UModularWheel::GetWheelRPM()
{
	return WheelState.Omega * 30.f / PI;
}

bool UModularWheel::IsWheelTouchingGround()
{
	return WheelState.HitResult.bBlockingHit;
}

FVector UModularWheel::GetWheelCenterLocation()
{
	return (WheelState.HitResult.bBlockingHit
		        ? WheelState.HitResult.ImpactPoint + (FVector(0, 0, WheelState.WheelSetup->WheelRadius))
		        : WheelState.HitResult.TraceEnd);
}

float UModularWheel::GetDampingForce()
{
	return WheelState.DampingForce;
}

void UModularWheel::ChangeTraceDebugVisbility(bool Enable)
{
	Debug = Enable;
}
