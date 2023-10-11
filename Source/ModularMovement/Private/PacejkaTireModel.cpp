//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "PacejkaTireModel.h"

#include "ModularMovementComponent.h"
#include "ModularWheel.h"
#include "Kismet/KismetMathLibrary.h"

void UPacejkaTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                         UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{
	Super::UpdateSimulation(DeltaTime, FinalForceVector, ModularMovementComponent, Wheel);
	// Gather necessary data
	const UPrimitiveComponent* Mesh=ModularMovementComponent->GetMesh();
	if(Wheel->ParentBodyOverride)
	{
		Mesh=Wheel->ParentBodyOverride;
	}
	const FTransform WorldTransform = Mesh->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	FVector WorldMeshVelocity = Mesh->GetBodyInstance()->
										   GetUnrealWorldVelocityAtPoint(
											   Wheel->WheelState.HitResult.TraceStart);
	WorldMeshVelocity.Z = 0;
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity / 100.f);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);


	const float WheelRadius = Wheel->WheelState.WheelSetup->WheelRadius / 100;
	const float WheelVelocity = Wheel->WheelState.AngularVelocity * WheelRadius;


	float SlipForward, SlipLateral;
	// Check if we are actually touching the ground
	if (Wheel->WheelState.HitResult.bBlockingHit)
	{
		if (GroundVelocityVector.Size() < 0.01)
		{
			Wheel->WheelState.SlipAngle = 0.f;
		}
		else
		{
			Wheel->WheelState.SlipAngle = FMath::Atan(
				GroundVelocityVector.Y / FMath::Abs(GroundVelocityVector.X + 5.f/*Denominator*/));
		}


		
		SlipForward = (GroundVelocityVector.X - WheelVelocity) / (FMath::Abs(GroundVelocityVector.X) + 5.f);

		SlipLateral = Wheel->WheelState.SlipAngle;


		Wheel->WheelState.SlipRatio = SlipForward;
	}
	else
	{
		//Not touching ground
		SlipForward = SlipLateral = 0.0f;
		Wheel->WheelState.SlipRatio = 0.f;
	}

	const float SinSlip = FMath::Sin(SlipLateral);
	const float TotalSlip = FMath::Sqrt(SlipForward * SlipForward + SinSlip * SinSlip);

	// calculate _skid and _reaction for sound.
	if (GroundVelocityVector.Size() < 0.02f)
	{
		Wheel->WheelState.TireStress = 0.f;
	}
	else
	{
		const float NewStress=UKismetMathLibrary::MapRangeClamped(TotalSlip,0.3,1,0,1);
		Wheel->WheelState.TireStress =UKismetMathLibrary::FInterpTo_Constant(Wheel->WheelState.TireStress, NewStress,DeltaTime,5);
		
	}
	if(TotalSlip>0.00001)
	{
		const float Sx_normalized = SlipForward / Speak;
		const float alpha_normalized = Wheel->WheelState.SlipAngle / SideSlipPeak;

		const float S_star = FMath::Sqrt(FMath::Square(Sx_normalized) + FMath::Square(alpha_normalized));

		// Step 4: Modified Slip Ratio and Slip Angle
		const float Sx_modified = S_star * Speak;
		const float alpha_modified = S_star * SideSlipPeak;

		FinalForceVector.X = Long.D * FMath::Sin(
			Long.C * FMath::Atan(Long.B * Sx_modified - Long.E * (Long.B * Sx_modified - FMath::Atan(Long.B * Sx_modified))));
		FinalForceVector.Y = Lat.D * FMath::Sin(
			Lat.C * FMath::Atan(
				Lat.B * alpha_modified - Lat.E * (Lat.B * alpha_modified - FMath::Atan(Lat.B * alpha_modified))));

		FinalForceVector.X = FinalForceVector.X * (Sx_normalized / S_star);
		FinalForceVector.Y = FinalForceVector.Y * (alpha_normalized / S_star);
	}

	//Surface friction
	float SurfaceFriction = 1.f;

	if (Wheel->WheelState.HitResult.PhysMaterial.IsValid())
	{
		SurfaceFriction = Wheel->WheelState.HitResult.PhysMaterial->Friction;
	}

	FinalForceVector.X *= Wheel->WheelState.WheelLoad.Z * SurfaceFriction;
	FinalForceVector.Y *= Wheel->WheelState.WheelLoad.Z * SurfaceFriction;

	RELAXATION2(FinalForceVector.X, LastFX, 50.0f);
	RELAXATION2(FinalForceVector.Y, LastFY, 50.0f);


	FinalForceVector *= -1.f;


	// Get the drive and brake torques
	 float DriveTorque = Wheel->WheelState.DriveTorque / WheelRadius;
	if(Wheel->WheelState.WheelSetup->TractionControl&&DriveTorque!=0.f&&GroundVelocityVector.X>0.5)
	{
		if(FMath::Abs(SlipForward)>Speak)
		{
			//release the brakes
			DriveTorque=DriveTorque*Speak/FMath::Abs(SlipForward);
		}
	}


	// The tire force affects wheel angular velocity by resisting the drive torque.
	const  float TireTorque = FinalForceVector.X / WheelRadius;



	// Approximate wheel inertia
	const float WheelInertia = Wheel->WheelState.WheelSetup->WheelMass * FMath::Square(WheelRadius) / 2;

	const float NetForces = DriveTorque - TireTorque;


	float AngularAcceleration = DeltaTime * NetForces / WheelInertia;


	Wheel->WheelState.AngularVelocity += AngularAcceleration;

	 float BrakeTorque = - FMath::Sign(Wheel->WheelState.AngularVelocity) * Wheel->WheelState.BrakeTorque;
	if(Wheel->WheelState.WheelSetup->ABS&&!Wheel->WheelState.IsHandBrakeTorque&&BrakeTorque!=0.f)
	{
		if(FMath::Abs(SlipForward)>Speak)
		{
			//release the brakes
			BrakeTorque=0.f;
		}
	}
	AngularAcceleration = DeltaTime * BrakeTorque / WheelInertia;

	if (FMath::Abs(AngularAcceleration) > FMath::Abs(Wheel->WheelState.AngularVelocity))
	{
		AngularAcceleration = -Wheel->WheelState.AngularVelocity;
	}

	Wheel->WheelState.AngularVelocity += AngularAcceleration;




	


	//Gather Debug
	const FVector TireForce = (FinalForceVector / Wheel->WheelState.WheelLoad.Size());
	TireForceNormalized = FVector2f(TireForce.X, TireForce.Y);
}

float UPacejkaTireModel::GetTireStress()
{
	return WheelOwner->WheelState.TireStress;
}

void UPacejkaTireModel::SetupWheels()
{
	Super::SetupWheels();

	float MaxForceX = 0.0f;
	float MaxForceY = 0.0f;
	Speak = SideSlipPeak = 0.f;


	
	// Loop through potential slip ratios and slip angles
	for (float SlipRatio = 0.0f; SlipRatio <= 1.0f; SlipRatio += 0.05f)
	{
		// Calculate forces using your Pacejka formula


		// Check if this force is the maximum force in X direction
		if (const float ForceX = 	 Long.D * FMath::Sin(
			Long.C * FMath::Atan(Long.B * SlipRatio - Long.E * (Long.B * SlipRatio - FMath::Atan(Long.B * SlipRatio)))); ForceX > MaxForceX)
		{
			MaxForceX = ForceX;
			Speak = SlipRatio;
		}
	}
	//radians increments by 1 degree 
	for (float SlipAngle = 0.0f; SlipAngle <= 0.5f; SlipAngle += 0.0174533f)
	{
		// Check if this force is the maximum force in Y direction
		if (const float ForceY = Lat.D * FMath::Sin(
			Lat.C * FMath::Atan(
				Lat.B * SlipAngle - Lat.E * (Lat.B * SlipAngle - FMath::Atan(Lat.B * SlipAngle)))); ForceY > MaxForceY)
		{
			MaxForceY = ForceY;
			SideSlipPeak = SlipAngle;
		}
	}
	
}

FString UPacejkaTireModel::GetTireDebugData(FVector2f& SlipData)
{
	SlipData = TireForceNormalized;
	FString Output = "SlipRatio: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipRatio) + TEXT("\n");
	Output += "SlipAngle: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipAngle) + TEXT("\n");
	//Add Drive torque
	Output += "DriveTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque) + TEXT("\n");
	//Add normalized tire force
	Output += "NetTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque*WheelOwner->GetWheelSetup()->WheelRadius/100- TireForceNormalized.X*WheelOwner->WheelState.WheelLoad.Z);
	// Draw Text at wheel location
	DrawDebugString(WheelOwner->GetWorld(),WheelOwner->GetComponentLocation(),Output,nullptr,FColor::Red,0.0f,true);
	return Output;
}
