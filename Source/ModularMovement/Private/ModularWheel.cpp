//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularMovement.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Kismet/KismetMathLibrary.h"

#include "PhysicsEngine/PhysicsConstraintComponent.h"


DECLARE_CYCLE_STAT(TEXT("Modular Updage Suspension"), STAT_ModularSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Forces"), STAT_ModularForces, STATGROUP_MovementPhysics);

// Sets default values for this component's properties
UModularWheel::UModularWheel(): WheelState(), ParentBodyOverride(nullptr)
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
	//Initialize WheelSetup from WheelSetupClass
	if (WheelState.WheelSetupClass.LoadSynchronous())
	{
		WheelState.WheelSetup = NewObject<UModularVehicleWheelData>(this, WheelState.WheelSetupClass.Get());

		FTransform Transform = GetRelativeTransform();
		//Initialize data
		if (GetOwner()->IsChildActor())
		{
			Transform = GetComponentTransform().GetRelativeTransform(GetOwner()->GetParentActor()->GetTransform());
		}


		WheelState.InitialLocalLocation = Transform.GetLocation();
		if (GetAttachSocketName() != NAME_None)
		{
			WheelState.InitialLocalLocation += ModularMovementComponent->GetMesh()->GetSocketTransform(
				GetAttachSocketName(), RTS_Actor).GetTranslation();
		}
		WheelState.InitialLocalRotation = Transform.GetRotation().Rotator();
		const float SideAngle = WheelState.InitialLocalLocation.Y < 0 ? 1 : -1;
		WheelState.SuspAngle = UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(this) * SideAngle;
		//Duplicate the tiremodel assset
		if (WheelState.WheelSetup->TireModel)
		{
			WheelState.WheelSetup->TireModel = DuplicateObject<UBaseTireModel>(WheelState.WheelSetup->TireModel, this);
			GetTireModel()->SetupWheels();
		}else
		{
			UModularVehicleFunctionLibrary::NotifyError("No tire model. Please Select a tire model in your wheel setup");
		}

		ModularMovementComponent->ActorsToIgnore.AddUnique(GetOwner());

		if (GetOwner()->IsChildActor())
		{
			ModularMovementComponent->ActorsToIgnore.AddUnique(ModularMovementComponent->GetOwner());
		}
	}else
	{
		UModularVehicleFunctionLibrary::NotifyError("Wheel Setup class is missing in wheel components. Please create and assign one !");
	}


	if(OptionalBoneName!=NAME_None)
	{
		if(auto SMesh=Cast<USkeletalMeshComponent>(ModularMovementComponent->GetMesh()))
		{
			
			
			SMesh->SetAllBodiesBelowPhysicsDisabled(OptionalBoneName,true,true);
			
		}
	}
}

void UModularWheel::UpdateSuspension(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	if (!ModularMovementComponent || !WheelState.WheelSetup)
	{
		return;
	}


	MODULAR_CYCLE_COUNTER(STAT_ModularSuspension)


	
	// Gather data for trace
	WheelState.WheelLoad = FVector::ZeroVector;
	UPrimitiveComponent* Mesh = ModularMovementComponent->GetMesh();
	if (ParentBodyOverride)
	{
		Mesh = ParentBodyOverride;
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	const FVector ComponentLocation = MeshTransform.TransformPosition(
		WheelState.InitialLocalLocation + WheelState.WheelSetup->TraceStartOffset);
	FVector DirectionVector = Mesh->GetUpVector();
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
	                                       ModularMovementComponent->ActorsToIgnore,
	                                       Debug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
	                                       Hits, true);
	// Look for valid hits 
	for (auto Hit : Hits)
	{
		if (Hit.bBlockingHit)
		{
			const FVector Position = MeshTransform.InverseTransformPosition(Hit.ImpactPoint) - WheelState.
				InitialLocalLocation;
			if (FMath::Abs(Position.Y) < WheelSetup->WheelWidth && Hit.ImpactNormal.Z > 0)
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
		WheelState.HitResult.bBlockingHit = false;
		WheelState.HitResult.TraceEnd = TraceEnd;
		return;
	}


		// Calculate suspension force and damping 
		const float CurrentLen = FMath::Clamp<float>( TraceResult.Time,0.f,1.f);
		const float Stiffness = WheelSetup->SpringRate * (1 - CurrentLen) * SuspensionLenOver100;
		const float SuspensionDiff = (CurrentLen - WheelState.PreviousLen) * SuspensionLenOver100;

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
			WheelState.WheelLoad = ((FVector::UpVector * (Stiffness + WheelState.DampingForce)));

			//Draw Debugs

			DirectionVector = UKismetMathLibrary::VLerp(DirectionVector, FVector::UpVector,
														WheelState.WheelSetup->SteepSurfaceAssistance);
			FVector CorrectedForce = DirectionVector * SIForceToUnrealForce(WheelState.WheelLoad.Z);


			AddForceAtPosition(Mesh, TraceResult.ImpactPoint, CorrectedForce, NAME_None);
	
		}

		WheelState.PreviousLen = CurrentLen;
		WheelState.HitResult = TraceResult;
	}


void UModularWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularForces)

	if (!ModularMovementComponent || !WheelState.WheelSetup || !WheelState.WheelSetup->TireModel)
	{
		return;
	}


	FVector FinalForceVector = FVector::ZeroVector;


	WheelState.WheelSetup->TireModel->UpdateSimulation(DeltaTime, FinalForceVector, ModularMovementComponent, this);



	WheelState.AngularPosition += WheelState.AngularVelocity * DeltaTime;


	float IntegerPart = 0.f;
	WheelState.AngularPosition = FMath::Modf(WheelState.AngularPosition / (2 * PI), &IntegerPart) * (2 * PI);


	FinalForceVector = SIForceToUnrealForce(FinalForceVector);


	//Apply Forces to bodies 
	if (ModularMovementComponent->ShouldProcessPhysics())
	{
		FVector FrictionForceVector=FVector(0.f,0.f,0.f);
		FrictionForceVector += GetForwardVector() * FinalForceVector.X;
		FrictionForceVector += GetRightVector() * FinalForceVector.Y;

		//Calculate direction of force

		UPrimitiveComponent* Mesh = ModularMovementComponent->GetMesh();
		if (ParentBodyOverride)
		{
			Mesh = ParentBodyOverride;
		}

	
		if (!FrictionForceVector.ContainsNaN())
		{
			AddForceAtPosition(Mesh, WheelState.HitResult.TraceStart, FrictionForceVector, NAME_None);
		}
	}
}

void UModularWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent,
                                   float InNormSteering)
{
	if (WheelState.SteerScale != 0.f)
	{
		const float AISteerMultiplier = ModularMovementComponent->VehicleState.IsAIVehicle
			                                ? ModularMovementComponent->GetSetup()->GetAIMaxSteerMultiplier()
			                                : 1;


		const float WheelSide = WheelState.InitialLocalLocation.Y;

		float OutSteeringAngle = 0.f;

		switch (ModularMovementComponent->GetSetup()->GetSteerType())
		{
		case Ackermann:
			{
				float WheelBase;
				float TrackWidth;
				ModularMovementComponent->GetSetup()->GetAckermannValues(WheelBase, TrackWidth);

				// Ensure valid values for WheelBase and TrackWidth
				if (WheelBase <= 0 || TrackWidth <= 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Invalid WheelBase or TrackWidth"));
					break;
				}

				// Calculate the desired steering angle
				const float DesiredSteeringAngle = WheelState.WheelSetup->SteeringMaxAngle * InNormSteering;

				// Calculate the Ackermann angle
				float AckermannSteeringAngle;
				if (WheelSide > 0) // Right wheel
				{
					AckermannSteeringAngle = -1 * FMath::Atan(
						WheelBase / (TrackWidth / 2 - WheelBase / FMath::Tan(
							FMath::DegreesToRadians(DesiredSteeringAngle))));
				}
				else // Left wheel
				{
					AckermannSteeringAngle = FMath::Atan(
						WheelBase / (TrackWidth / 2 + WheelBase / FMath::Tan(
							FMath::DegreesToRadians(DesiredSteeringAngle))));
				}

				// Convert the angle back to degrees
				AckermannSteeringAngle = FMath::RadiansToDegrees(AckermannSteeringAngle);

				// Output the Ackermann steering angle
				OutSteeringAngle = AckermannSteeringAngle * AISteerMultiplier;
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
		float SteerSpeedScale = ModularMovementComponent->GetSetup()->GetSteerSpeedScaleForSpeed(
			ModularMovementComponent->VehicleState.ForwardSpeed * 0.036/*CmSToKmH*/);
		// if is counter steering relative to
		if (FMath::Sign(OutSteeringAngle) == FMath::Sign(ModularMovementComponent->VehicleState.SideSpeed))
		{
			auto Speed = FVector2D(ModularMovementComponent->VehicleState.SideSpeed,
			                       ModularMovementComponent->VehicleState.ForwardSpeed);
			Speed.Normalize();
			if (const float CounterSteerScale = FMath::Min(
				1, FMath::Abs(
					ModularMovementComponent->VehicleState.VehicleData->GetCounterSteerMultiplier() *
					FVector2D::DotProduct(Speed, FVector2D(1, 0)))); CounterSteerScale > SteerSpeedScale)
			{
				SteerSpeedScale = CounterSteerScale;
			}
		}
		WheelState.SteerAngle = OutSteeringAngle * WheelState.SteerScale * SteerSpeedScale;
	}
}

void UModularWheel::SetDriveTorqueOnWheels(float Force)
{
	if (WheelState.ApplyDriveForce)
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
	if (WheelState.ApplyDriveForce)
	{
		return WheelState.AngularVelocity;
	}
	return 0.0f;
}


void UModularWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	if (!WheelState.AnimateChildComponent)
	{
		return;
	}
	FVector Location;
	FRotator Rotation;

	UModularVehicleFunctionLibrary::GetWheelAnimationData(this, Location, Rotation, DeltaTime);

	Location=GetComponentTransform().TransformPosition(Location);
	TArray<USceneComponent*> Components;
	GetChildrenComponents(false, Components);
	for (USceneComponent* Mesh : Components)
	{
		if (Mesh->IsA(UMeshComponent::StaticClass()))
		{
			Rotation = (Rotation.Quaternion().Rotator());
			Mesh->SetRelativeRotation(Rotation);


		
			Mesh->SetWorldLocation(Location);
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

float UModularWheel::GetTrackSpeed() const
{
	return WheelState.AngularVelocity * WheelState.WheelSetup->WheelRadius;
}

float UModularWheel::GetTrackOffset(float CurrentOffset, float SpeedMultiplier) const
{
	CurrentOffset += SpeedMultiplier * WheelState.AngularVelocity * WheelState.WheelSetup->WheelRadius;


	if (CurrentOffset > 1.f)
	{
		CurrentOffset -= 1.f;
	}
	if (CurrentOffset < 0)
	{
		CurrentOffset += 1.f;
	}

	return CurrentOffset;
}


UBaseTireModel* UModularWheel::GetTireModel()
{
	return GetWheelSetup()->TireModel;
}

void UModularWheel::ChangeTraceDebugVisibility(bool Enable)
{
	Debug = Enable;
}

Chaos::FRigidBodyHandle_Internal* UModularWheel::GetInternalHandle(const UPrimitiveComponent* Component, FName BoneName)
{
	if (IsValid(Component))
	{
		if (const FBodyInstance* BodyInstance = Component->GetBodyInstance(BoneName))
		{
			if (const auto Handle = BodyInstance->ActorHandle)
			{
				if (Chaos::FRigidBodyHandle_Internal* RigidHandle = Handle->GetPhysicsThreadAPI())
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
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 WorldTorque = Chaos::FVec3::CrossProduct(Position - WorldCOM, Force);
		RigidHandle->AddForce(Force, false);
		RigidHandle->AddTorque(WorldTorque, false);
	}
}
