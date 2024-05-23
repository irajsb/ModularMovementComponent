//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ArcadeTireModel.h"

#include "ModularMovementComponent.h"

UArcadeTireModel::UArcadeTireModel()
{
	RebuildCurves(false);
}
#if WITH_EDITOR
void UArcadeTireModel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildCurves(AutoGenerateTheGraph);
}
#endif

void UArcadeTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                        UPrimitiveComponent* Mesh, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{
	Super::UpdateSimulation(DeltaTime, FinalForceVector, Mesh, ModularMovementComponent, Wheel);
	//Gather necessary data
	
	if (Wheel->ParentBodyOverride)
	{
		Mesh = Wheel->ParentBodyOverride;
	}
	const FTransform WorldTransform = Mesh->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	 FVector WorldMeshVelocity = Mesh->GetBodyInstance()->
	                                        GetUnrealWorldVelocityAtPoint(
		                                        Wheel->WheelState.HitResult.TraceStart);
	WorldMeshVelocity.Z = 0;
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	
	const float WheelRadiusM = Wheel->WheelState.WheelSetup->WheelRadius / 100.f;

	const float MassPerWheel = ModularMovementComponent->GetMassPerWheel();
	const float WheelLoad=UseConstantWheelLoad?MassPerWheel*10.f:Wheel->WheelState.WheelLoad.Size() ;

	bool Locked = false;
	bool Spinning = false;
	//Check if braking
	const bool Braking = FMath::Abs(Wheel->WheelState.DriveTorque) < FMath::Abs(Wheel->WheelState.BrakeTorque);

	//Clamp for friction 
	const float ForceRequiredToBringToStop = FMath::Abs(
		MassPerWheel * FrictionMultiplierLongitudinal * (GroundVelocityVector.X) / 100 /
		DeltaTime);

	
	// are we actually touching the ground
	if (Wheel->WheelState.HitResult.bBlockingHit)
	{

		//Surface friction
		float SurfaceFriction = 1.f;

		if (Wheel->WheelState.HitResult.PhysMaterial.IsValid())
		{
			SurfaceFriction = Wheel->WheelState.HitResult.PhysMaterial->Friction;
		}
		
		//Friction of phys mat is default 0.7
		const float LongitudinalAdhesiveLimit = WheelLoad * SurfaceFriction * FrictionMultiplierLongitudinal;
		const float LateralFrictionLoadMultiplier = WheelLoad * SurfaceFriction;

		if (Braking)
		{
			//Clamp brake torque to force required to bring to stop, helps in lower speeds
			FinalForceVector.X = Wheel->WheelState.BrakeTorque * -1 * FMath::Sign(GroundVelocityVector.X) /
				WheelRadiusM;
			FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -ForceRequiredToBringToStop,
			                                  ForceRequiredToBringToStop);

			//if ABS enabled we allow unrealistic brake torque because of arcade tire model 
			if (!Wheel->WheelState.WheelSetup->ABS || Wheel->WheelState.IsHandBrakeTorque)
			{
				Locked = FMath::Abs(FinalForceVector.X) > LongitudinalAdhesiveLimit;
				FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -LongitudinalAdhesiveLimit,
				                                  LongitudinalAdhesiveLimit);
			}
			else
			{
				Locked = false;
			}
		}
		else
		{
			Spinning = (FMath::Abs(Wheel->WheelState.DriveTorque) / WheelRadiusM > LongitudinalAdhesiveLimit) && !Wheel
				->WheelState.WheelSetup->TractionControl;
			FinalForceVector.X = Wheel->WheelState.DriveTorque / WheelRadiusM;
			FinalForceVector.X = FMath::Clamp(FinalForceVector.X, -LongitudinalAdhesiveLimit,
			                                  LongitudinalAdhesiveLimit);
		}

		//Lateral friction
		Wheel->WheelState.SlipAngle = FMath::Atan(
			GroundVelocityVector.Y / FMath::Abs(GroundVelocityVector.X + 5.f/*Denominator*/));
	
		const float LatMultiplier=LateralGripCurve.GetRichCurve()->Eval(FMath::Abs(Wheel->WheelState.SlipAngle));
		const float ForceRequiredToBringToStopLateral = FMath::Abs(MassPerWheel * LatMultiplier * (GroundVelocityVector.Y) / 100 /DeltaTime);
		
		FinalForceVector.Y = LatMultiplier *LateralFrictionLoadMultiplier * FMath::Sign(GroundVelocityVector.Y) * -1;
		FinalForceVector.Y=FMath::Clamp(FinalForceVector.Y,-ForceRequiredToBringToStopLateral,ForceRequiredToBringToStopLateral);

	
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

	const float LongitudinalStress = Spinning || Locked;
	//ignore in low speeds
	const float LateralStress = GroundVelocityVector.Size() > 500.f ? Wheel->WheelState.SlipAngle : 0;
	Wheel->WheelState.TireStress = FMath::Clamp(FMath::Max(LongitudinalStress, LateralStress), 0.f, 1.f);

	//Gather Debug
	const FVector TireForce = (FinalForceVector / WheelLoad);
	TireForceNormalized = FVector2f(TireForce.X, TireForce.Y);



	

}

FString UArcadeTireModel::GetTireDebugData(FVector2f& SlipData)
{
	SlipData = TireForceNormalized;
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


void UArcadeTireModel::RebuildCurves(bool Force)
{
	if (LateralGripCurve.GetRichCurve()->IsEmpty() || AutoGenerateTheGraph)
	{
		if (LateralGripCurve.ExternalCurve)
		{
			return;
		}
		LateralGripCurve.GetRichCurve()->Reset();
		const auto LateralForceCurve = LateralGripCurve.GetRichCurve();
		auto KeyHandle = LateralForceCurve->AddKey(0, 0);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, RCIM_Cubic);
		KeyHandle = LateralForceCurve->AddKey(0.1396263, MaxFrictionLateralForce);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, RCIM_Cubic);
		KeyHandle = LateralForceCurve->AddKey(0.3490659, MinFrictionLateralForce);
		LateralForceCurve->SetKeyInterpMode(KeyHandle, RCIM_Cubic);
	}
}
