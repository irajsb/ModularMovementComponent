// Fill out your copyright notice in the Description page of Project Settings.


#include "PropellerModel.h"


#include "ModularWheel.h"


double UPropellerModel::CalculateThrust(double Ct, double rho, double A, double omega)
{
	
	return Ct * rho * A * FMath::Pow(omega, 2);
	
}

void UPropellerModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                       UPrimitiveComponent* Mesh, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{


	Wheel->WheelState.HitResult.TraceStart=Wheel->GetComponentLocation();
	Super::UpdateSimulation(DeltaTime, FinalForceVector, Mesh, ModularMovementComponent, Wheel);


	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	
	
	
	Wheel->GetWheelState()->HitResult.bBlockingHit=true; 
	// Calculate angular acceleration (Alpha) from engine torque and opposing forces on the propeller such as drag
	const float  EngineTorque = Wheel->WheelState.DriveTorque;
	const float EngineSign=FMath::Sign(EngineTorque);
	const float  OpposingTorque =Wheel->WheelState.BrakeTorque+ 0.5 * DragCoefficient * Rho * AreaOfResistance * FMath::Pow(Wheel->WheelState.AngularVelocity, 2) * PropellerRadius;
	const float  AngularAcceleration = (EngineTorque - (OpposingTorque*FMath::Sign(Wheel->WheelState.AngularVelocity))) / MomentOfInertia;

	// Calculate change in angular velocity
	const float  DeltaOmega = AngularAcceleration * DeltaTime;

	// Update angular velocity of the propeller
	const float  NewOmega = Wheel->WheelState.AngularVelocity + DeltaOmega;

	

	// Update the wheel state with the new angular velocity
	Wheel->WheelState.AngularVelocity = NewOmega;


	

	// Calculate thrust force
	const float  ThrustForce = CalculateThrust(Ct, Rho, A, NewOmega);

	// Apply force in the direction of propeller rotation
	FinalForceVector = FVector(ThrustForce*FMath::Sign(Wheel->WheelState.AngularVelocity), 0, 0);
	FinalForceVector=SteeringRotator.UnrotateVector(FinalForceVector);

	
	// Determine if braking
	const bool Braking = FMath::Abs(Wheel->WheelState.DriveTorque) < FMath::Abs(Wheel->WheelState.BrakeTorque);
	TireForceNormalized=FinalForceVector;
	// Apply braking force if braking
	if (Braking)
		FinalForceVector -= FVector(0, 0, FMath::Abs(Wheel->WheelState.BrakeTorque));
}

FString UPropellerModel::GetTireDebugData(FVector2f& SlipData)
{
	
		
		FString Output = "SlipRatio: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipRatio) + TEXT("\n");
		Output += "SlipAngle: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipAngle) + TEXT("\n");
		//Add Drive torque
		Output += "DriveTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque) + TEXT("\n");
		//Add normalized tire force
		Output += "NetTorque: " + FString::SanitizeFloat(
			WheelOwner->WheelState.DriveTorque * WheelOwner->GetWheelSetup()->WheelRadius / 100 - TireForceNormalized.X *
			WheelOwner->WheelState.WheelLoad.Z);
#if ENABLE_DRAW_DEBUG
		// Draw Text at wheel location
		DrawDebugString(WheelOwner->GetWorld(), WheelOwner->GetComponentLocation(), Output, nullptr, FColor::Red, 0.0f,
						true);
						
#endif
		return Output;
	
	
}
