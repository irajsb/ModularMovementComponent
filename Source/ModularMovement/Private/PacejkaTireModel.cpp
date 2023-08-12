// Fill out your copyright notice in the Description page of Project Settings.


#include "PacejkaTireModel.h"

#include "ModularMovementComponent.h"
#include "ModularWheel.h"
#include "VisualLogger.h"
#include "VisualLoggerTypes.h"
#include "Kismet/KismetMathLibrary.h"

void UPacejkaTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                         UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{
	Super::UpdateSimulation(DeltaTime, FinalForceVector, ModularMovementComponent, Wheel);
	// Gather necessary data 
	const FTransform WorldTransform = ModularMovementComponent->GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	const FVector WorldMeshVelocity = ModularMovementComponent->GetMesh()->GetBodyInstance()->
																GetUnrealWorldVelocityAtPoint(
																	Wheel->WheelState.HitResult.TraceStart);
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity/100.f);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	const float SpeedSign = FMath::Sign(GroundVelocityVector.X);
	
	const float WheelRadius = Wheel->WheelState.WheelSetup->WheelRadius / 100;
	const float WheelVelocity = Wheel->WheelState.AngularVelocity * WheelRadius;
	const float MassPerWheel = (ModularMovementComponent->GetMesh()->GetMass() / ModularMovementComponent->
		GetNumberOfWheels());



	float SlipX=0.f,SlipY=0.f;
	// Check if we are actually touching the ground
	if (Wheel->WheelState.HitResult.bBlockingHit)
	{
		if(GroundVelocityVector.Size()<0.01)
		{
			Wheel->WheelState.SlipAngle =0.f;
		}else
		{
			Wheel->WheelState.SlipAngle = FMath::Atan(
				GroundVelocityVector.Y / FMath::Abs(GroundVelocityVector.X + 5.f/*Denominator*/));
		}
		

			SlipX = (GroundVelocityVector.X - WheelVelocity) / (FMath::Abs(GroundVelocityVector.X)+5.f);
			SlipY = FMath::Sin(Wheel->WheelState.SlipAngle);
			
		
	LastSlipX=SlipX;

		
	}else
	{//Not touching ground
	LastSlipX=	SlipX = SlipY = 0.0f;
	}
	
	float Ft = 0.0f;
	float Fn = 0.0f;
	const float TotalSlip = FMath::Sqrt(SlipX*SlipX+SlipY*SlipY);
	
	// calculate _skid and _reaction for sound.
	if (GroundVelocityVector.Size() < 0.02f) {
		Wheel->WheelState.TireStress=0.f;
	} else {

		//Wheel->WheelState.TireStress =  TotalSlip;
	}

	const float stmp = FMath::Min(TotalSlip, 1.5f);;

	// MAGIC FORMULA
	const float Bx = PacejkaBLong * stmp;
	 float F = FMath::Sin(PacejkaCLong * FMath::Atan(Bx * (1.0f - PacejkaELong) + PacejkaELong * FMath::Atan(Bx))) * (1.0f + stmp *1.f*PacejkaDLong);
	//float FLat = FMath::Sin(PacejkaCLat * FMath::Atan(Bx * (1.0f - PacejkaELat) + PacejkaELat * FMath::Atan(Bx))) * (1.0f + stmp *1.f*PacejkaDLat);
	
	// load sensitivity
	const float mu =FMath::Min(1.f, MU *UKismetMathLibrary::MapRangeClamped(Wheel->WheelState.WheelLoad.Z ,0, MassPerWheel,LoadFactorMin,LoadFactorMax));

	//Surface friction
	float SurfaceFriction=1.f;
	
	if(Wheel->WheelState.HitResult.PhysMaterial.IsValid())
	{
		SurfaceFriction=Wheel->WheelState.HitResult.PhysMaterial->Friction;
	}
	F *= Wheel->WheelState.WheelLoad.Z * mu *SurfaceFriction  ;//Camber wheel * (1.0f + 0.05f * sin(-wheel->staticPos.ax * 18.0f));	/* coeff */
	//FLat*=Wheel->WheelState.WheelLoad.Z * mu *SurfaceFriction;
	if (TotalSlip > 0.000001f) {
		// wheel axis based
		Ft -= F * SlipX / TotalSlip;
		Fn -= F * SlipY*2 / TotalSlip;
		
	}
	RELAXATION2(Fn, LastFn, 50.0f);
	RELAXATION2(Ft, LastFt, 50.0f);

	UE_LOG(LogTemp,Log,TEXT("SlipY %f TotaLSlip %f"),SlipY,TotalSlip);
	
	
	FinalForceVector.X = Ft;
	FinalForceVector.Y= -Fn;

	
	FinalForceVector.Y = FinalForceVector.Y * -1;
	// Get the drive and brake torques
	const float driveTorque = Wheel->WheelState.DriveTorque;
	const float brakeDirection = FMath::Sign(Wheel->WheelState.AngularVelocity);
	// The tire force affects wheel angular velocity by resisting the drive torque.
	const float tireTorque = FinalForceVector.X * WheelRadius ;

	

	// Approximate wheel inertia
	const float wheelInertia = Wheel->WheelState.WheelSetup->WheelMass * FMath::Square(WheelRadius) / 2;

	const float NetForces=driveTorque  - tireTorque;




	
	float AngularAcceleration=0.f;

	AngularAcceleration = DeltaTime *NetForces/ wheelInertia;
	UE_LOG(LogTemp,Log,TEXT("driveTorque %f tireTorque %f "),driveTorque,tireTorque);
	
	Wheel->WheelState.AngularVelocity += AngularAcceleration;
	

	const float BrakeTorque = - FMath::Sign(Wheel->WheelState.AngularVelocity) * Wheel->WheelState.BrakeTorque;
	AngularAcceleration = DeltaTime * BrakeTorque / wheelInertia;

	if (FMath::Abs(AngularAcceleration) > FMath::Abs(Wheel->WheelState.AngularVelocity)) {
		AngularAcceleration = -Wheel->WheelState.AngularVelocity;
	}

	Wheel->WheelState.AngularVelocity += AngularAcceleration;
	

	//Gather Debug
	const FVector TireForce=(FinalForceVector/Wheel->WheelState.WheelLoad.Size());
	TireForceNormalized=FVector2f(TireForce.X,TireForce.Y);
}

float UPacejkaTireModel::GetTireStress()
{
	return TireStress;
}

FString UPacejkaTireModel::GetTireDebugData(FVector2f& SlipData)
{
	SlipData=TireForceNormalized;
	FString Output = "SlipRatio: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipRatio)+TEXT("\n");
	Output+="SlipAngle: "+FString::SanitizeFloat(WheelOwner->WheelState.SlipAngle)+TEXT("\n");
	return Output;
}
