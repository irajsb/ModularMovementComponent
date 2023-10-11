//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "TankTireModel.h"

#include "ModularMovementComponent.h"
#include "ModularWheel.h"

void UTankTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                      UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{
	Super::UpdateSimulation(DeltaTime, FinalForceVector, ModularMovementComponent, Wheel);
	
	const float TrackInput=Wheel->WheelState.InitialLocalLocation.Y>0.f?ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer
	:ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer;

	const UPrimitiveComponent* Mesh=ModularMovementComponent->GetMesh();
	if(Wheel->ParentBodyOverride)
	{
		Mesh=Wheel->ParentBodyOverride;
	}
	//Gather necessary data 
	const FTransform WorldTransform = Mesh->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	 FVector WorldMeshVelocity = Mesh->GetBodyInstance()->
																GetUnrealWorldVelocityAtPoint(
																	Wheel->WheelState.HitResult.TraceStart);
	WorldMeshVelocity.Z = 0;
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	const float MassPerWheel = (Mesh->GetMass() / ModularMovementComponent->
		GetNumberOfWheels());
	const float WheelRadiusM =SprocketRadius / 100.f;

	
	bool Locked=false;
	bool Spinning=false;
	//Check if braking or idle braking

	const float WrongDirectionThreshold=ModularMovementComponent->VehicleState.VehicleData->GetWrongDirectionThreshold();
	const bool Braking =FMath::Abs(GroundVelocityVector.X)>WrongDirectionThreshold&&( FMath::Sign(GroundVelocityVector.X)*FMath::Sign(TrackInput)==-1.f||TrackInput==0.f);

	
	//Clamp for friction 
	const float ForceRequiredToBringToStop = FMath::Abs(
		MassPerWheel *FrictionMultiplierLongitudinal * (GroundVelocityVector.X) / 100 /
		DeltaTime);



	const float ReverseVelocitySign=-1.f * FMath::Sign(GroundVelocityVector.X);

	// are we actually touching the ground
	if (Wheel->WheelState.HitResult.bBlockingHit)
	{
		//Friction of phys mat is default 0.7
		const float LongitudinalAdhesiveLimit = Wheel->WheelState.WheelLoad.Size() * Wheel->WheelState.HitResult.
			PhysMaterial.Get()->Friction * FrictionMultiplierLongitudinal;
		const float LateralFrictionLimit = Wheel->WheelState.WheelLoad.Size() * Wheel->WheelState.HitResult.
			PhysMaterial.Get()->Friction*FrictionMultiplierLateral;

		if (Braking)
		{
			float BrakeInput=FMath::Abs(TrackInput);
			if(TrackInput==0.f)
			{
				BrakeInput=NormalizedSteeringBrakeInput;
			}
			
			//Clamp brake torque to force required to bring to stop, helps in lower speeds
			FinalForceVector.X = BrakeInput*Wheel->WheelState.WheelSetup->BrakeTorque *ReverseVelocitySign /WheelRadiusM;
			
			
			//if ABS enabled we allow unrealistic brake torque because of arcade tire model 
			if(!Wheel->WheelState.WheelSetup->ABS||Wheel->WheelState.IsHandBrakeTorque)
			{
				Locked = FMath::Abs(FinalForceVector.X) > LongitudinalAdhesiveLimit;
				FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -LongitudinalAdhesiveLimit,
												  LongitudinalAdhesiveLimit);
			}
			else
			{
				Locked=false;
			}
			
		}
		else
		{
			Spinning = (FMath::Abs(Wheel->WheelState.DriveTorque)/WheelRadiusM > LongitudinalAdhesiveLimit)&&!Wheel->WheelState.WheelSetup->TractionControl;
			FinalForceVector.X = Wheel->WheelState.DriveTorque/WheelRadiusM*TrackInput;
			FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -LongitudinalAdhesiveLimit,
			                                  LongitudinalAdhesiveLimit);
			
		}
		
	
		
		FinalForceVector.Y = (MassPerWheel * -FrictionMultiplierLateral * LocalWheelVelocity.Y/100.f);
		
		FinalForceVector.Y=FMath::Clamp(FinalForceVector.Y,-LateralFrictionLimit,LateralFrictionLimit);
		FinalForceVector.Y=FMath::Clamp(FinalForceVector.Y,-ForceRequiredToBringToStop,ForceRequiredToBringToStop);
		
		

		
	}

	//Determine wheel speed
	if (Locked)
	{
		Wheel->WheelState.AngularVelocity = 0;
	}
	else
	{
		const float GroundOmega = GroundVelocityVector.X / Wheel->WheelState.WheelSetup->WheelRadius;
		Wheel->WheelState.AngularVelocity = (GroundOmega);
	}

	const float LongitudinalStress=static_cast<float>(Spinning||Locked);
	//ignore in low speeds
	const float LateralStress=GroundVelocityVector.Size()>500.f? Wheel->WheelState.SlipAngle:0;
	Wheel->WheelState.TireStress=FMath::Clamp(FMath::Max(LongitudinalStress,LateralStress),0.f,1.f);
	const FVector TireForce = (FinalForceVector / Wheel->WheelState.WheelLoad.Size());
	TireForceNormalized = FVector2f(TireForce.X, TireForce.Y);

}

FString UTankTireModel::GetTireDebugData(FVector2f& SlipData)
{
	
	SlipData = TireForceNormalized;
	FString Output = "SlipRatio: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipRatio) + TEXT("\n");
	Output += "SlipAngle: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipAngle) + TEXT("\n");
	//Add Drive torque
	Output += "DriveTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque) + TEXT("\n");
	//Add normalized tire force
	Output += "NetTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque*WheelOwner->GetWheelSetup()->WheelRadius/100- TireForceNormalized.X*WheelOwner->WheelState.WheelLoad.Z);
	// Draw Text at wheel location

	return Output;
}
