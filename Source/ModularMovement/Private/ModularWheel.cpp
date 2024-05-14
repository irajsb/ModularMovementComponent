//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularMovement.h"
#include "ModularVehicleData.h"
#include "VehicleParticleSurfaceData.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Kismet/KismetMathLibrary.h"

#include "PhysicsEngine/PhysicsConstraintComponent.h"


DECLARE_CYCLE_STAT(TEXT("Modular Updage Suspension"), STAT_ModularSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Forces"), STAT_ModularForces, STATGROUP_MovementPhysics);

// Sets default values for this component's properties
UModularWheel::UModularWheel(): WheelState(), ParentBodyOverride(nullptr), SurfaceData(nullptr),
                                NoFrictionDefaultPhysMaterial(nullptr),
                                MovementComponentRef(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UModularWheel::BeginPlay()
{
	Super::BeginPlay();

	GetChildrenComponents(false, ChildWheels);
	if(GetWheelSetup()->SuspensionType==Constraint)
	{
		
		
		for (auto Component:ChildWheels)
		{	if(Component!=ConstraintParent)
		{
			FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
			Component->AttachToComponent(ConstraintParent,AttachmentRules);
		}
		}
	}

	if(IsValid(SurfaceDataClass))
	{
		SurfaceData=DuplicateObject<UVehicleParticleSurfaceData>(SurfaceDataClass.GetDefaultObject(),this);
	}
}

void UModularWheel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(SurfaceData)
	{
		SurfaceData->UpdateParticleForWheel(this);
	}
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
		}
		else
		{
			UModularVehicleFunctionLibrary::NotifyError(
				"No tire model. Please Select a tire model in your wheel setup");
		}

		ModularMovementComponent->ActorsToIgnore.AddUnique(GetOwner());

		if (GetOwner()->IsChildActor())
		{
			ModularMovementComponent->ActorsToIgnore.AddUnique(ModularMovementComponent->GetOwner());
		}
	}
	else
	{
		UModularVehicleFunctionLibrary::NotifyError(
			"Wheel Setup class is missing in wheel components. Please create and assign one !");
	}


	if (OptionalBoneName != NAME_None)
	{
		if (auto SMesh = Cast<USkeletalMeshComponent>(ModularMovementComponent->GetMesh()))
		{
			SMesh->SetAllBodiesBelowPhysicsDisabled(OptionalBoneName, true, true);
		}
	}
}

void UModularWheel::UpdateSuspension(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	MovementComponentRef=ModularMovementComponent;
	if (!ModularMovementComponent || !WheelState.WheelSetup)
	{
		return;
	}
	if (WheelState.WheelSetup->WheelRadius == 0.f)
	{
		return;
	}

	WheelState.WheelLoad = FVector::ZeroVector;

	if (WheelState.WheelSetup->SuspensionType == Constraint)
	{
		FVector Lin, Ang;
		SuspensionConstraint->GetConstraintForce(Lin, Ang);
		
		WheelState.WheelLoad.Z = FMath::Abs(Lin.Z);

		WheelState.HitResult.TraceStart = GetComponentLocation();
		if (WheelState.WheelLoad.Z > 100.f)
		{
			WheelState.HitResult.bBlockingHit = true;
		}


		if (!WheelCollision)
		{
			return;
		}
	}
	MODULAR_CYCLE_COUNTER(STAT_ModularSuspension)


	//if we have a custom wheel collision then use that for tracing as well. It will be used only for friction 

	// Gather data for trace

	UPrimitiveComponent* Mesh = ModularMovementComponent->GetMesh();
	if (ParentBodyOverride)
	{
		Mesh = ParentBodyOverride;
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	const FVector ComponentLocation = WheelCollision
		                                  ? WheelCollision->GetComponentLocation()
		                                  : MeshTransform.TransformPosition(
			                                  WheelState.InitialLocalLocation + WheelState.WheelSetup->
			                                  TraceStartOffset);
	FVector DirectionVector = Mesh->GetUpVector();
	const FVector TraceEnd = ComponentLocation + (DirectionVector * -1 * WheelState.WheelSetup->SuspensionLength);
	FHitResult TraceResult;
	TraceResult.TraceStart = ComponentLocation;
	TraceResult.TraceEnd = WheelCollision ? ComponentLocation : TraceEnd;
	TraceResult.bBlockingHit = false;
	TArray<FHitResult> Hits;
	bool ValidHitFound = false;

	const auto WheelSetup = GetWheelSetup();
	const float SuspensionLenOver100 = WheelSetup->SuspensionLength / 100;

	// Start Trace
	UKismetSystemLibrary::SphereTraceMulti(GetWorld(), ComponentLocation, TraceResult.TraceEnd, WheelState.WheelSetup->WheelRadius,
	                                       ModularMovementComponent->GetSetup()->GetSuspensionTraceTypeQuery(), true,
	                                       ModularMovementComponent->ActorsToIgnore,
	                                       Debug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
	                                       Hits, true);
	// Look for valid hits 
	for (auto Hit : Hits)
	{
		if (Hit.bBlockingHit)
		{
			
			const FVector Position = MeshTransform.InverseTransformPosition(Hit.ImpactPoint) - WheelState.InitialLocalLocation;
			if (FMath::Abs(Position.Y) < WheelSetup->WheelWidth ||WheelCollision )
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


	if (WheelState.WheelSetup->SuspensionType != Constraint)
	{
		// Calculate suspension force and damping 
		const float CurrentLen = FMath::Clamp<float>(TraceResult.Time, 0.f, 1.f);
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
		
	}
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
		FVector ImpactToLocation =   WheelState.HitResult.ImpactPoint-WheelState.HitResult.Location;
		ImpactToLocation.Normalize();
		FVector::CrossProduct(ImpactToLocation, GetRightVector());

		const FVector GroundZVector = WheelState.HitResult.Normal;
		const FVector GroundXVector = FVector::CrossProduct(GetRightVector(), GroundZVector);
		const FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
		const FMatrix Mat = FMatrix(GroundXVector, GroundYVector, GroundZVector, ModularMovementComponent->GetMesh()->GetComponentLocation());
		const FVector FrictionForceVector = Mat.TransformVector(FinalForceVector);
		

		//Calculate direction of force

		UPrimitiveComponent* Mesh = ModularMovementComponent->GetMesh();
		if (ParentBodyOverride)
		{
			Mesh = ParentBodyOverride;
		}

		
		if (!FrictionForceVector.ContainsNaN())
		{

			if(!SuspensionConstraint){
			AddForceAtPosition(Mesh, WheelState.HitResult.TraceStart, FrictionForceVector, NAME_None);

			}else
			{
				
				AddForceAtPosition(ConstraintParent, WheelState.HitResult.ImpactPoint, FrictionForceVector, NAME_None);
			}
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

	Location = GetComponentTransform().TransformPosition(Location);
	

	for (USceneComponent* Mesh : ChildWheels)
	{
		if (Mesh->IsA(UMeshComponent::StaticClass()))
		{
			Rotation = (Rotation.Quaternion().Rotator());
			Mesh->SetRelativeRotation(Rotation);

			if(!SuspensionConstraint)
			{
				Mesh->SetWorldLocation(Location);
			}
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
	if (WheelState.WheelSetup)
	{
		return WheelState.WheelSetup;
	}

	return Cast<UModularVehicleWheelData>(WheelState.WheelSetupClass.LoadSynchronous()->ClassDefaultObject);
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

void UModularWheel::SetActiveDifferentialIndex(uint8 Index, UModularMovementComponent* MovementComponent)
{
	auto Data = Cast<UModularVehicleData>(MovementComponent->VehicleState.VehicleData);
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("No vehicle data in SetActiveDifferentialIndex"))
		return;
	}
	if (!Data->DifferentialData.IsValidIndex(Index) )
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid index  in SetActiveDifferentialIndex"))
		return;
	}if(DifferentialBlackList.Contains(Index))
	{
		UE_LOG(LogTemp, Error, TEXT("Differential is blacklisted for this wheel. Ignoring"))
		Index=255;
	}

	if(Data->DifferentialData.IsValidIndex(DifferentialIndex))
	{
		Data->DifferentialData[DifferentialIndex].Wheels.Remove(this);
	}
	DifferentialIndex = Index;
	if(Data->DifferentialData.IsValidIndex(DifferentialIndex))
	{
		Data->DifferentialData[DifferentialIndex].Wheels.Add(this);
	}
}

uint8 UModularWheel::GetActiveDifferentialIndex()
{
	return DifferentialIndex;
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

void UModularWheel::SetupConstraints(UModularMovementComponent* MovementComponent, UPrimitiveComponent* ParentBody,
                                     UPrimitiveComponent* WheelOrDifferential, UPrimitiveComponent* InWheelCollision)
{
	if (SuspensionConstraint)
	{
		SuspensionConstraint->DestroyComponent();
	}
	auto WheelSetup = GetWheelSetup();
	if (WheelSetup)
	{
		if (WheelSetup->SuspensionType == Constraint)
		{
			const FTransform WheelTransform = GetComponentTransform();
			const FTransform ParentTransform = MovementComponent->GetOwner()->GetTransform();
			const FTransform RelativeTransform = WheelTransform.GetRelativeTransform(ParentTransform);
			auto Comp = MovementComponent->GetOwner()->AddComponentByClass(
				UPhysicsConstraintComponent::StaticClass(), false, RelativeTransform, false);
			SuspensionConstraint = Cast<UPhysicsConstraintComponent>(Comp);
			SuspensionConstraint->SetConstrainedComponents(WheelOrDifferential, NAME_None, ParentBody, NAME_None);
			ConstraintParent=WheelOrDifferential;
			// Set up the constraint properties here
			SuspensionConstraint->SetLinearXLimit(LCM_Limited, 1.f);
			SuspensionConstraint->SetLinearYLimit(LCM_Limited, 1.f);
			SuspensionConstraint->SetLinearZLimit(LCM_Limited, WheelSetup->SuspensionLength);
			
			SuspensionConstraint->SetAngularTwistLimit(ACM_Free, 0);
			SuspensionConstraint->SetAngularSwing1Limit(ACM_Free, 0);
			SuspensionConstraint->SetAngularSwing2Limit(ACM_Locked, 0);
			SuspensionConstraint->SetLinearPositionTarget(FVector(0, 0, -WheelSetup->SuspensionLength));
			SuspensionConstraint->SetLinearPositionDrive(true, true, true);
			
			
			SuspensionConstraint->GetConstraint().Get()->SetDisableCollision(true);
			SuspensionConstraint->SetLinearVelocityDrive(true, true, true);
			SuspensionConstraint->SetLinearVelocityTarget(FVector(0, 0, 0));

			auto Constraint=	SuspensionConstraint->GetConstraint().Get();

			//Suspension and velocity forces * By a multiplier in Twist 
			const float S=WheelSetup->SpringRate;
			const float V=WheelSetup->DampingCompress;
			const float M=WheelSetup->NonZForceMultiplier;
			Constraint->SetLinearDriveParams(FVector(S*M,S*M,S),FVector(V*M,V*M,V),FVector::ZeroVector);

			WheelCollision = InWheelCollision;
			NoFrictionDefaultPhysMaterial = NewObject<UPhysicalMaterial>();
			NoFrictionDefaultPhysMaterial->Friction = 0.f;
			NoFrictionDefaultPhysMaterial->StaticFriction = 0.f;
			NoFrictionDefaultPhysMaterial->Restitution = 0.5;
			NoFrictionDefaultPhysMaterial->FrictionCombineMode = EFrictionCombineMode::Min;
			if (WheelCollision)
			{
				WheelCollision->SetPhysMaterialOverride(NoFrictionDefaultPhysMaterial);
			}
			// You can adjust other properties like drive properties, angular limits, etc.
		}
	}
}

void UModularWheel::CallCustomEvent(uint8 Index)
{
	if(MovementComponentRef)
	{
		MovementComponentRef->OnCustomEvent.Broadcast(Index,this);
	}
}
