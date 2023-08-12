// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularMovement.h"
#include "SingleParticlePhysicsProxy.h"
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
    
    // Gather data for trace
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

    const auto WheelSetup = GetWheelSetup();
    const float SuspensionLenOver100 = WheelSetup->SuspensionLength / 100;

    // Start Trace
    UKismetSystemLibrary::SphereTraceMulti(GetWorld(), ComponentLocation, TraceEnd, WheelState.WheelSetup->WheelRadius,
                                           ModularMovementComponent->GetSetup()->GetSuspensionTraceTypeQuery(), true,
                                           ActorsToIgnore, Debug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
                                           Hits, true);
    // Look for valid hits 
    for (auto Hit : Hits)
    {
        if (Hit.bBlockingHit)
        {
            const FVector Position = MeshTransform.InverseTransformPosition(Hit.ImpactPoint) - WheelState.InitialLocalLocation;
            if (FMath::Abs(Position.Y) < WheelSetup->WheelWidth)
            {
                ValidHitFound = true;
                TraceResult = Hit;
                break;
            }
        }
    }

    if (!ValidHitFound)
    {
        // Handle the situation where no valid hits were found, such as logging a warning or taking alternative actions
        UE_LOG(LogTemp, Warning, TEXT("No valid hits found in suspension update."));
    	WheelState.HitResult.bBlockingHit=false;
    	WheelState.HitResult.TraceEnd=TraceEnd;
        return;
    }

    // Calculate suspension force and damping 
    const float CurrentLen = FMath::Max<float>(0, TraceResult.Time);
    const float Stiffness = WheelSetup->SpringRate * (1 - CurrentLen) * SuspensionLenOver100;
    const float SuspensionDiff = (CurrentLen - WheelState.PreviousLen) * SuspensionLenOver100;
	Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(ModularMovementComponent->GetMesh(), NAME_None);
    if (CurrentLen - WheelState.PreviousLen < 0)
    {
        WheelState.DampingForce = -1 * (((SuspensionDiff) * WheelSetup->DampingCompress)) / DeltaTime;
    }
    else
    {
        WheelState.DampingForce = -1 * (((SuspensionDiff) * WheelSetup->DampingRebound)) / DeltaTime;
    }

    if (TraceResult.bBlockingHit && ModularMovementComponent->ShouldProcessPhysics())
    {
        const float AngleCorrection = (FVector::DotProduct(TraceResult.ImpactNormal,
                                                           (TraceResult.TraceStart - TraceResult.TraceEnd).
                                                           GetUnsafeNormal()));
        WheelState.WheelLoad = ((AngleCorrection * FVector::UpVector * (Stiffness + WheelState.DampingForce)));
       
    	AddForceAtPosition(ModularMovementComponent->GetMesh(),TraceResult.TraceStart,  SIForceToUnrealForce(WheelState.WheelLoad),NAME_None);
    	
    }

    WheelState.PreviousLen = CurrentLen;
    WheelState.HitResult = TraceResult;
}

void UModularWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularForces)
	
	if (!ModularMovementComponent||!WheelState.WheelSetup->TireModel)
	{
		return;
	}

	WheelState.WheelStatus = Normal;
	FVector FinalForceVector = FVector::ZeroVector;
	

	WheelState.WheelSetup->TireModel->UpdateSimulation(DeltaTime,FinalForceVector,ModularMovementComponent,this);
	
	
		
	WheelState.AngularPosition += WheelState.AngularVelocity * DeltaTime;
	
	
	float IntegerPart = 0.f;
	WheelState.AngularPosition = FMath::Modf(WheelState.AngularPosition / (2*PI), &IntegerPart) *( 2*PI);

	
	FinalForceVector = SIForceToUnrealForce(FinalForceVector);
	const float SteerAngleDegrees = WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);

	//Apply Forces to bodies 
	if (ModularMovementComponent->ShouldProcessPhysics())
	{
		FVector FrictionForceLocal = FinalForceVector;
		FrictionForceLocal =SteeringRotator.RotateVector(FrictionForceLocal);
		const FVector GroundZVector = WheelState.HitResult.ImpactNormal;
		const FVector GroundXVector = FVector::CrossProduct(ModularMovementComponent->GetMesh()->GetRightVector(),
		                                                    GroundZVector);
		const FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
		const FMatrix Mat(GroundXVector, GroundYVector, GroundZVector,
		                  ModularMovementComponent->GetMesh()->GetComponentLocation());
		const FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);


		
		AddForceAtPosition(ModularMovementComponent->GetMesh(),WheelState.HitResult.TraceStart,FrictionForceVector,NAME_None);
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
		return WheelState.AngularVelocity;
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
				//TODO:
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
		
		const FVector SuspensionTraceLocation = WheelState.HitResult.bBlockingHit
			                                        ? WheelState.HitResult.Location 
			                                        : WheelState.HitResult.TraceEnd;


		const FTransform WheelTransform = GetWheelTransform();
		//local
		
		float Sin, Cos;

		const FVector ContactPointPosition = WheelTransform.InverseTransformPosition(SuspensionTraceLocation);
		

		FVector ResultPosition = FVector::ZeroVector;
		ResultPosition.Z = ContactPointPosition.Z ;


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
	return WheelState.AngularVelocity * 30.f / PI;
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

float UModularWheel::GetTireStress()
{
	
	return WheelState.TireStress;
}

UBaseTireModel* UModularWheel::GetTireModel()
{
	return GetWheelSetup()->TireModel;
}

void UModularWheel::ChangeTraceDebugVisbility(bool Enable)
{
	Debug = Enable;
}

Chaos::FRigidBodyHandle_Internal* UModularWheel::GetInternalHandle(UPrimitiveComponent* Component, FName BoneName)
{
	if(IsValid(Component))
	{
		if(const FBodyInstance* BodyInstance = Component->GetBodyInstance(BoneName))
		{
			if(const auto Handle = BodyInstance->ActorHandle)
			{
				if(Chaos::FRigidBodyHandle_Internal* RigidHandle = Handle->GetPhysicsThreadAPI())
				{
					return RigidHandle;
				}
			}
		}
	}
	return nullptr;
}

void UModularWheel::AddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName)
{
	if(Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 WorldTorque = Chaos::FVec3::CrossProduct(Position - WorldCOM, Force);
		RigidHandle->AddForce(Force, false);
		RigidHandle->AddTorque(WorldTorque, false);
	}
}