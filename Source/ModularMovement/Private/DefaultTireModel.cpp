//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "DefaultTireModel.h"




//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ArcadeTireModel.h"

#include "ModularMovementComponent.h"
#include "ModularMovementPhysicalMaterial.h"
#include "Kismet/KismetMathLibrary.h"

UDefaultTireModel::UDefaultTireModel()
{
	RebuildCurves(false,false);
}

#if WITH_EDITOR
void UDefaultTireModel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildCurves(AutoGenerateGraph,AutoGenerateTheLateralGraph);
}

#endif
void UDefaultTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                         UPrimitiveComponent* Mesh, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{

	Super::UpdateSimulation(DeltaTime, FinalForceVector, Mesh, ModularMovementComponent, Wheel);
	// Gather necessary data 
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

	const bool Combine=UseCombinedFriction||(Wheel->WheelState.IsHandBrakeTorque&&UseCombinedFrictionWhenHandBraking);
	const float MassPerWheel = ModularMovementComponent->GetMassPerWheel();
	const float WheelLoad=UseConstantWheelLoad?MassPerWheel*10.f:Wheel->WheelState.WheelLoad.Size() ;

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
		
		const float NewStress=UKismetMathLibrary::MapRangeClamped(TotalSlip,0.3,0.6,0,1);
		
			Wheel->WheelState.TireStress=NewStress;
		
	}
	if(TotalSlip>0.00001)
	{
		if(Combine)
		{
			const float Sx_normalized = FMath::Abs( SlipForward )/ Speak;
			const float alpha_normalized =FMath::Abs( Wheel->WheelState.SlipAngle) / SideSlipPeak;

			float S_star = FMath::Sqrt(FMath::Square(Sx_normalized) + FMath::Square(alpha_normalized));

			// Step 4: Modified Slip Ratio and Slip Angle
			float Sx_modified = S_star * Speak;
			float alpha_modified = S_star * SideSlipPeak;

		
			FinalForceVector.X = LongitudinalGripCurve.GetRichCurve()->Eval(FMath::Abs(Sx_modified))*FMath::Sign(SlipForward);

			const float LatMultiplier=LateralGripCurve.GetRichCurve()->Eval(FMath::Abs(Wheel->WheelState.SlipAngle));
			
		
		
			
			FinalForceVector.Y =LateralGripCurve.GetRichCurve()->Eval(alpha_modified)*FMath::Sign(Wheel->WheelState.SlipAngle);

			FinalForceVector.X = FinalForceVector.X * (Sx_normalized / S_star);
			FinalForceVector.Y = FinalForceVector.Y * (alpha_normalized / S_star);
		}else
		{
			FinalForceVector.X = LongitudinalGripCurve.GetRichCurve()->Eval(FMath::Abs(SlipForward))*FMath::Sign(SlipForward);
			FinalForceVector.Y =LateralGripCurve.GetRichCurve()->Eval(Wheel->WheelState.SlipAngle)*FMath::Sign(Wheel->WheelState.SlipAngle);
		}
	
	}


	

	//Surface friction
	float SurfaceFriction = 1.f;
	if (const auto PhysMat=Wheel->GetActivePhysicalMaterial())
	{
		
		SurfaceFriction = PhysMat->Friction;
		if(const UModularMovementPhysicalMaterial* Material=Cast<UModularMovementPhysicalMaterial>(PhysMat))
		{
			const float AngularVelocity= Wheel->WheelState.AngularVelocity;
			const float LinearVelocity = AngularVelocity * WheelRadius;
			const float  DragForce = 0.5f * Material->DragCoefficient  * LinearVelocity * LinearVelocity;
			
			Wheel->WheelState.BrakeTorque+= DragForce * WheelRadius;

			ModularMovementComponent->UseCustomDrag=true;
			ModularMovementComponent->CustomDragCoefficient=Material->BodyDragCoefficient;
			ModularMovementComponent->UpdateAirDrag();
		}
		else
		{
			ModularMovementComponent->UseCustomDrag=false;
		}
	}

	FinalForceVector.X *= WheelLoad * SurfaceFriction;
	FinalForceVector.Y *= WheelLoad* SurfaceFriction;

	//RELAXATION2(FinalForceVector.X, LastFX, 50.0f);
	//RELAXATION2(FinalForceVector.Y, LastFY, 50.0f);


	FinalForceVector *= -1.f;


	// Get the drive and brake torques
	const float DriveTorque = Wheel->WheelState.DriveTorque / WheelRadius;


	// The tire force affects wheel angular velocity by resisting the drive torque.
	const float TireTorque = FinalForceVector.X / WheelRadius;


	// Approximate wheel inertia
	const float WheelInertia = Wheel->WheelState.WheelSetup->WheelMass * FMath::Square(WheelRadius) / 2;

	const float NetForces = DriveTorque - TireTorque;


	float AngularAcceleration = DeltaTime * NetForces / WheelInertia;


	Wheel->WheelState.AngularVelocity += AngularAcceleration;


	 float BrakeTorque = - FMath::Sign(Wheel->WheelState.AngularVelocity) * Wheel->WheelState.BrakeTorque;
	
	if(Wheel->WheelState.WheelSetup->ABS&&!Wheel->WheelState.IsHandBrakeTorque)
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
	const FVector TireForce = (FinalForceVector / WheelLoad);
	
	TireForceNormalized = FVector2f(TireForce.X, TireForce.Y);
	
}

FString UDefaultTireModel::GetTireDebugData(FVector2f& SlipData)
{
	SlipData = TireForceNormalized;
	FString Output = "SlipRatio: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipRatio) + TEXT("\n");
	Output += "SlipAngle: " + FString::SanitizeFloat(WheelOwner->WheelState.SlipAngle) + TEXT("\n");
	//Add Drive torque
	Output += "DriveTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque) + TEXT("\n");
	//Add normalized tire force
	Output += "NetTorque: " + FString::SanitizeFloat(WheelOwner->WheelState.DriveTorque*WheelOwner->GetWheelSetup()->WheelRadius/100- TireForceNormalized.X*WheelOwner->WheelState.WheelLoad.Z)+ TEXT("\n");;
	Output += "WheelLoad: " + FString::SanitizeFloat(WheelOwner->WheelState.WheelLoad.Z);
#if ENABLE_DRAW_DEBUG
	// Draw Text at wheel location
	DrawDebugString(WheelOwner->GetWorld(),WheelOwner->GetComponentLocation(),Output,nullptr,FColor::Red,0.0f,true);
#endif
	return Output;
}


void UDefaultTireModel::SetupWheels()
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
		if (const float ForceX = 	LongitudinalGripCurve.GetRichCurve()->Eval(FMath::Abs(SlipRatio)); ForceX > MaxForceX)
		{
			MaxForceX = ForceX;
			Speak = SlipRatio;
		}
	}
	//radians increments by 1 degree 
	for (float SlipAngle = 0.0f; SlipAngle <= 0.5f; SlipAngle += 0.0174533f)
	{
		// Check if this force is the maximum force in Y direction
		if (const float ForceY = LateralGripCurve.GetRichCurve()->Eval(FMath::Abs(SlipAngle)); ForceY > MaxForceY)
		{
			MaxForceY = ForceY;
			SideSlipPeak = SlipAngle;
		}
	}
	
}

void UDefaultTireModel::RefreshTireData()
{
	Super::RefreshTireData();
	RebuildCurves(AutoGenerateGraph,AutoGenerateTheLateralGraph);
}


void UDefaultTireModel::RebuildCurves(bool ForceLong,bool ForceLateral)
{
	if(LateralGripCurve.GetRichCurve()->IsEmpty()||ForceLateral)
	{
		if(!LateralGripCurve.ExternalCurve)
		{
			LateralGripCurve.GetRichCurve()->Reset();
			const auto LateralForceCurve=LateralGripCurve.GetRichCurve();
			auto KeyHandle = LateralForceCurve->AddKey(0, 0);
			LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
			KeyHandle = LateralForceCurve->AddKey(0.01, MaxFrictionLateralForce);
			LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
			KeyHandle = LateralForceCurve->AddKey(0.3490659, MinFrictionLateralForce);
			LateralForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
		}
	}

	if(LongitudinalGripCurve.GetRichCurve()->IsEmpty()||ForceLong)
	{
		if(!LongitudinalGripCurve.ExternalCurve)
		{
			LongitudinalGripCurve.GetRichCurve()->Reset();
			const auto ForceCurve=LongitudinalGripCurve.GetRichCurve();
			auto KeyHandle = ForceCurve->AddKey(0, 0);
			ForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
			KeyHandle = ForceCurve->AddKey(0.02, MaxFrictionForce);
			ForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
			KeyHandle = ForceCurve->AddKey(1.f, MinFrictionForce);
			ForceCurve->SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic);
		}
	}
}
