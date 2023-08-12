// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeTireModel.h"

#include "ModularMovementComponent.h"

UArcadeTireModel::UArcadeTireModel()
{
	RebuildCurves(false);
}

void UArcadeTireModel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildCurves(AutoGenerateTheGraph);
}

void UArcadeTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                        UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{

	Super::UpdateSimulation(DeltaTime, FinalForceVector, ModularMovementComponent, Wheel);
	//Gather necessary data 
	const FTransform WorldTransform = ModularMovementComponent->GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	const FVector WorldMeshVelocity = ModularMovementComponent->GetMesh()->GetBodyInstance()->
	                                                            GetUnrealWorldVelocityAtPoint(
		                                                            Wheel->WheelState.HitResult.TraceStart);
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	const float MassPerWheel = (ModularMovementComponent->GetMesh()->GetMass() / ModularMovementComponent->
		GetNumberOfWheels());
	const float WheelRadiusM = Wheel->WheelState.WheelSetup->WheelRadius / 100.f;

	
	bool Locked=false;
	bool Spinning=false;
	//Check if braking
	const bool Braking = FMath::Abs(Wheel->WheelState.DriveTorque) < FMath::Abs(Wheel->WheelState.BrakeTorque);

	//Clamp for friction 
	const float ForceRequiredToBringToStop = FMath::Abs(
		MassPerWheel *FrictionMultiplierLongitudinal * (GroundVelocityVector.X) / 100 /
		DeltaTime);

	// are we actually touching the ground
	if (Wheel->WheelState.HitResult.bBlockingHit)
	{
		//Friction of phys mat is default 0.7
		const float LongitudinalAdhesiveLimit = Wheel->WheelState.WheelLoad.Size() * Wheel->WheelState.HitResult.
			PhysMaterial.Get()->Friction * FrictionMultiplierLongitudinal;
		const float LateralFrictionLoadMultiplier = Wheel->WheelState.WheelLoad.Size() * Wheel->WheelState.HitResult.
			PhysMaterial.Get()->Friction;

		if (Braking)
		{
			//Clamp brake torque to force required to bring to stop, helps in lower speeds
			FinalForceVector.X = Wheel->WheelState.BrakeTorque * -1 * FMath::Sign(GroundVelocityVector.X)/WheelRadiusM;
			FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -ForceRequiredToBringToStop,
											  ForceRequiredToBringToStop);
			
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
			FinalForceVector.X = Wheel->WheelState.DriveTorque/WheelRadiusM;
			FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -LongitudinalAdhesiveLimit,
			                                  LongitudinalAdhesiveLimit);
		}

		//Lateral friction
		Wheel->WheelState.SlipAngle = FMath::Atan(GroundVelocityVector.Y / FMath::Abs(GroundVelocityVector.X + 5.f/*Denominator*/));
		//
		FinalForceVector.Y =LateralGripCurve.GetRichCurve()->Eval(FMath::Abs(Wheel->WheelState.SlipAngle)) *
			LateralFrictionLoadMultiplier * FMath::Sign(GroundVelocityVector.Y) * -1;

	
		if (Locked||Spinning)
		{
			//TODO Add these params
			FinalForceVector.Y *= 0.3;
			FinalForceVector.X*=0.6;
		}

		
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
	
}



void UArcadeTireModel::RebuildCurves(bool Force)
{
	if(LateralGripCurve.GetRichCurve()->IsEmpty()||AutoGenerateTheGraph)
	{
		if(LateralGripCurve.ExternalCurve)
		{
			return;
		}
		LateralGripCurve.GetRichCurve()->Reset();
		const auto LateralForceCurve=LateralGripCurve.GetRichCurve();
		auto KeyHandle = LateralForceCurve->AddKey(0, 0);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
		KeyHandle = LateralForceCurve->AddKey(0.1396263, MaxFrictionLateralForce);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
		KeyHandle = LateralForceCurve->AddKey(0.3490659, MinFrictionLateralForce);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
	}
}
