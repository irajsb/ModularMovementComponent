// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeMovementComponent.h"

#include "ArcadePawn.h"
#include "ArcadeWheelInterface.h"
#include "TitanMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
#include "PhysicsEngine/PhysicsSettings.h"
FArcadeVehicleDebugParams GArcadeVehicleDebugParams;

DECLARE_CYCLE_STAT(TEXT("Arcade Tick Component"), STAT_ArcadeTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Engine"), STAT_ArcadeEngine, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Suspension"), STAT_ArcadeSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Forces"), STAT_ArcadeForces, STATGROUP_MovementPhysics);


static FAutoConsoleVariableRef CVarArcadeVehicleShowSuspensionDebug(
    TEXT("ArcadeVehicle.ShowSuspensionDebug"),
    GArcadeVehicleDebugParams.ShowSuspensionDebug,
    TEXT("Toggles Suspension Debugging visuals"));
static FAutoConsoleVariableRef CVarArcadeVehicleShowInputProcessLog(
    TEXT("ArcadeVehicle.ShowInputProcessLog"),
    GArcadeVehicleDebugParams.ShowInputProcessingDebug,
    TEXT("Toggles Input Debugging UE_LOGs"));

static FAutoConsoleVariableRef CVarArcadeVehicleShowGearBoxProcessLog(
    TEXT("ArcadeVehicle.ShowGearBoxProcessLog"),
    GArcadeVehicleDebugParams.ShowGearboxLog,
    TEXT("Toggles GearBox Debugging UE_LOGs"));
static FAutoConsoleVariableRef CVarArcadeVehicleFrictionDraw(
    TEXT("ArcadeVehicle.ShowFriction"),
    GArcadeVehicleDebugParams.ShowDrawFriction,
    TEXT("Toggles Friction force "));

#define LOCTEXT_NAMESPACE "ArcadeMovement"

FORCEINLINE float OmegaToRPM(float Omega)
{
	return Omega * 30.f / PI;
}

UArcadeMovementComponent::UArcadeMovementComponent()
{
	AHUD::OnShowDebugInfo.AddUObject(this, &UArcadeMovementComponent::ShowDebugInfo);
}

UMeshComponent* UArcadeMovementComponent::GetMesh()
{
	return  Cast<UMeshComponent>(UpdatedComponent);
}

void UArcadeMovementComponent::SetInputThrottle(float Input)
{
	RawThrottleInput=FMath::Clamp<float>(Input,-1.f,1.f);
}

void UArcadeMovementComponent::SetInputSteering(float Input)
{
	RawSteeringInput=FMath::Clamp<float>(Input,-1.f,1.f);
}

void UArcadeMovementComponent::SetBrakeInput(float Brake)
{
	RawBrakeInput = FMath::Clamp(Brake, -1.0f, 1.0f);
}

int UArcadeMovementComponent::GetNumberOfWheels()
{//TODo
	return 4;
}

FArcadeGearInfo UArcadeMovementComponent::GetGearInfo(int Index)
{
	if(VehicleState.VehicleData->Gears.IsValidIndex(Index))
	{
		return VehicleState.VehicleData->Gears[Index];
	}else
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("Wrong GearIndex"));
		FMessageLog("Blueprint").Warning(LOCTEXT("GearIndexNotValid", "Passed Gear Index was not valid"));
	}
	const 	FArcadeGearInfo Gear(1);
	return  Gear;
}

void UArcadeMovementComponent::InitializeComponent()
{


	Super::InitializeComponent();
if(!VehicleState.VehicleData)
{
	UE_LOG(LogArcadeVehicle,Error,TEXT("Assign The Vehicle DataAsset "));
	FMessageLog("Blueprint").Error(LOCTEXT("VehicleDataAssetNotValid", "Passed Assign Vehicle Data Asset "));
	return;
}
	
	Components=	GetOwner()->GetComponentsByInterface(UArcadeWheelInterface::StaticClass());
	//finding Idle
	for (int Index=0 ;Index!=VehicleState.VehicleData->Gears.Num();++Index)
	{
		if (VehicleState.VehicleData->Gears[Index].GearRatio==0)
		{
			VehicleState.IdleGear=Index;
			//TODO Temp
		VehicleState.TargetGear=	VehicleState.CurrentGear=VehicleState.IdleGear+1;
		}
	}

	//Setting up Wheel initital Data
	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->SetupWheels(this);
	}
	
}

void UArcadeMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeTickComponent)
	const float fDeltaTime=FMath::Min<float>(DeltaTime,0.0333);
	if(!VehicleState.VehicleData)
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("Assign The Vehicle DataAsset "));
		FMessageLog("Blueprint").Error(LOCTEXT("VehicleDataAssetNotValid", "Passed Assign Vehicle Data Asset "));
		return;
	}
	
	
	
	
	UpdateState(fDeltaTime);
	UpdateEngine(fDeltaTime);
	UpdateSuspension(fDeltaTime);
	UpdateSteering(fDeltaTime);
	UpdateGearBox(fDeltaTime);
	UpdateForces(fDeltaTime);
}

void UArcadeMovementComponent::UpdateState(float DeltaTime)
{
	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetPhysicsLinearVelocity(), GetMesh()->GetForwardVector());
}

void UArcadeMovementComponent::UpdateGearBox(float DeltaTime)
{

	if (VehicleState.VehicleData->bReverseAsBrake)
	{
		//for reverse as state we want to automatically shift between reverse and first gear
		if (FMath::Abs(VehicleState.ForwardSpeed) < VehicleState.VehicleData->WrongDirectionThreshold)	//we only shift between reverse and first if the car is slow enough.
			{
			if (RawBrakeInput > KINDA_SMALL_NUMBER &&VehicleState.CurrentGear >= VehicleState.IdleGear && VehicleState.TargetGear >= VehicleState.IdleGear)
			{
				SetTargetGear(-1, false);
			}
			else if (RawThrottleInput > KINDA_SMALL_NUMBER &&VehicleState.CurrentGear <= VehicleState.IdleGear && VehicleState.TargetGear <= VehicleState.IdleGear)
			{
				SetTargetGear(1, false);
			}
			}
	}
	/*else
	{
		if (PVehicle->GetTransmission().Setup().TransmissionType == Chaos::ETransmissionType::Automatic
            && RawThrottleInput > KINDA_SMALL_NUMBER
            && PVehicle->GetTransmission().GetCurrentGear() == 0
            && PVehicle->GetTransmission().GetTargetGear() == 0)
		{
			SetTargetGear(1, true);
		}
	}*/

	//simulate



	// not currently changing gear, also don't want to change up because the wheels are spinning up due to having no load
	if (VehicleState.CurrentGearChangeTime==0.f /*&& AllowedToChangeGear*/)
	{
		if (VehicleState.CurrentRpmRatio >= GetGearInfo(VehicleState.CurrentGear).UpRatio)
		{
			SetTargetGear(VehicleState.CurrentGear+1,false);
		}
		else if (VehicleState.CurrentRpmRatio <=  GetGearInfo(VehicleState.CurrentGear).DownRatio && VehicleState.CurrentGear > VehicleState.IdleGear+1) // don't change down to neutral
			{
			SetTargetGear(VehicleState.CurrentGear-1,false);
			}
	}


	if (VehicleState.CurrentGear != VehicleState.TargetGear)
	{
		VehicleState.CurrentGearChangeTime -= DeltaTime;
		if (VehicleState.CurrentGearChangeTime <= 0.f)
		{
			VehicleState.CurrentGearChangeTime = 0.f;
			VehicleState.CurrentGear =VehicleState.TargetGear;
			if(GArcadeVehicleDebugParams.ShowGearboxLog)
				UE_LOG(LogArcadeVehicle,Warning,TEXT("Change gear Timer Finished Gear Now at : %d "),VehicleState.CurrentGear);
		}
	}
}

void UArcadeMovementComponent::UpdateEngine(float DeltaTime)
{

	ARCADE_CYCLE_COUNTER(STAT_ArcadeEngine)
	float HighestOmega=0;
	for(UActorComponent* Component: Components)
	{
	
	const float ComponentOmega=	FMath::Abs(Cast<IArcadeWheelInterface>(Component)->GetFastestWheelOmegaSpeed());
		if(ComponentOmega>HighestOmega)
			HighestOmega=ComponentOmega;
	}

	const float ThrottleInput=CalcThrottleInput();
	const float WheelRPM =OmegaToRPM(HighestOmega);
	VehicleState.CurrentRpm=FMath::Clamp<float>(WheelRPM *GetGearInfo(VehicleState.CurrentGear).GearRatio*VehicleState.VehicleData->DifferentialRatio,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
	VehicleState.CurrentRpmRatio= UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm,0,1);
	const float EngineTorque= VehicleState.VehicleData->ConstantTorque!=0.0?VehicleState.VehicleData->ConstantTorque*ThrottleInput:ThrottleInput* VehicleState.VehicleData->EngineTorqueCurve.GetRichCurve()->Eval(VehicleState.CurrentRpm);
	const float TransmissionTorque=GetGearInfo(VehicleState.CurrentGear).GearRatio* VehicleState.VehicleData->TransmissionEfficiency;
	const float WheelTorque=EngineTorque*TransmissionTorque;
	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->SetDriveTorqueOnWheels(WheelTorque);
	}
	
}



void UArcadeMovementComponent::UpdateSuspension(float DeltaTime)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeSuspension)

	

	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateSuspension(DeltaTime,this);
	}
}

void UArcadeMovementComponent::UpdateForces(float DeltaTime)
{

	ARCADE_CYCLE_COUNTER(STAT_ArcadeForces)


	

	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateForces(DeltaTime,this);
	}

}

void UArcadeMovementComponent::UpdateSteering(float DeltaTime)
{

	const float SteeringInput=CalcSteeringInput();
	
	const float SteerSpeedScale =VehicleState.VehicleData->SteerCurve.GetRichCurve()->Eval(VehicleState.ForwardSpeed*0.036/*CmSToKmH*/) ;
	
	float UseSteeringValue = SteeringInput ;//TODo* SteerSpeedScale;
	UE_LOG(LogTemp,Error,TEXT("Steer %f"),UseSteeringValue);

	
	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateSteering(DeltaTime,this,UseSteeringValue);
	}
	
}

void UArcadeMovementComponent::CalculateSteeringAngle(FWheelState& WheelState, float DeltaTime, USceneComponent* ArcadeWheel,float InNormSteering)
{
	float SteeringAngle = 0.f;
	/*if (FMath::Abs(GWheeledVehicleDebugParams.SteeringOverride) > 0.01f)
	{
	SteeringAngle = PWheel.Setup().WheelState.WheelSetup->SteeringMaxAngle * GWheeledVehicleDebugParams.SteeringOverride;
	}
	else*/
	{
		//
		
		float WheelSide =WheelState.LocalLocation.Y;
		


		//GetSteeringAngleChaos


		float OutSteeringAngle = 0.f;

		switch (VehicleState.VehicleData->SteerType)
		{
		case EArcadeSteerType::AngleRatio:
			{
				bool OutsideWheel = (InNormSteering * WheelSide) > 0.f;
				OutSteeringAngle = InNormSteering * (OutsideWheel ? WheelState.WheelSetup->SteeringMaxAngle : WheelState.WheelSetup->SteeringMaxAngle *0.7/* Setup().AngleRatio*/);

			}
			break;
//Steering system by mikasa ackermann
		case EArcadeSteerType::Ackermann:
			{
				/*FVector2D PtA; FVector2D PtB; float SteerLHS; float SteerRHS;
					
				Ackermann.CalculateAkermannAngle(-InNormSteering, SteerLHS, SteerRHS);
				
				OutSteeringAngle = (WheelSide < 0.0f) ? -SteerLHS : SteerRHS;
				OutSteeringAngle *= (WheelState.WheelSetup->SteeringMaxAngle / Ackermann.GetMaxAckermanAngle());*/
			}
			break;

		default:
        case EArcadeSteerType::SingleAngle:
			{
				OutSteeringAngle = WheelState.WheelSetup->SteeringMaxAngle * InNormSteering;
			}
			break;

		}

		
		//
		WheelState.SteerAngle=OutSteeringAngle;
	}

	
	
}



void UArcadeMovementComponent::WheelTrace(
                                          FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel)
{

	//logic
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	const FVector ComponentLocation=ArcadeWheel->GetComponentLocation()+WheelState.WheelSetup->TraceStartOffset;
	FHitResult TraceResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(),ComponentLocation,ComponentLocation+(ArcadeWheel->GetUpVector()*-1*WheelState.WheelSetup->SuspensionLength),WheelState.WheelSetup->WheelRadius,VehicleState.VehicleData->SuspensionTraceTypeQuery,true,ActorsToIgnore,GArcadeVehicleDebugParams.ShowSuspensionDebug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,TraceResult,true);

	const float CurrentLen=1-TraceResult.Time;
	const float DampingCorrection=((CurrentLen-WheelState.PreviousLen)*VehicleState.VehicleData->DampingCorrectionMultiplier*WheelState.WheelSetup->Stiffness);
	if(TraceResult.bBlockingHit)
	{
	WheelState.WheelLoad=DeltaTime*(FVector(0,0,1)*(WheelState.WheelSetup->Stiffness+DampingCorrection)*(CurrentLen));
	GetMesh()->AddForceAtLocation(WheelState.WheelLoad,TraceResult.TraceStart);
	
	}
	
	WheelState.PreviousLen=CurrentLen;
	WheelState.HitResult=TraceResult;
	//logic
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(GArcadeVehicleDebugParams.ShowSuspensionDebug)
	{
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelState.WheelSetup->Stiffness)*(CurrentLen)))/20,FColor::Red,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,20,0),TraceResult.TraceStart+FVector(0,20,0)+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelState.WheelSetup->Stiffness+DampingCorrection)*(CurrentLen)))/20,FColor::Green,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,-20,0),TraceResult.TraceStart+FVector(0,-20,0)+FVector(0,0,1)*DampingCorrection/20,FColor::Blue,false,-1,0,5);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,0),TEXT("OrginalForce"),0,FColor::Red,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-25),TEXT("CorrectedForce"),0,FColor::Green,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-50),TEXT("DampingCorrection"),0,FColor::Blue,0);

	}


	#endif

}

void UArcadeMovementComponent::ApplyWheelForces(FWheelState& WheelState, float DeltaTime,
    USceneComponent* ArcadeWheel)
{
	if(!WheelState.HitResult.bBlockingHit)
		return;

	const FTransform WorldTransform = GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = WheelState.SteerAngle; // temp
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	const FVector	LocalWheelVelocity = WorldTransform.InverseTransformVector(GetMesh()->GetPhysicsLinearVelocityAtPoint(ArcadeWheel->GetComponentLocation()));
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	//TODo const float SlipAngle = FMath::Atan2(GroundVelocityVector.Y, GroundVelocityVector.X);
	float FinalLongitudinalForce = 0.f;
	FVector ForceFromFriction;
	//EffectiveRadius
	const float Re=WheelState.WheelSetup->WheelRadius;
	const float MassPerWheel=(GetMesh()->GetMass()/GetNumberOfWheels());
	float 	AppliedLinearDriveForce = WheelState.DriveTorque * CmToM(Re);
	float AppliedLinearBrakeForce = WheelState.BrakeTorque* CmToM(Re);

		// currently just letting the brake override the throttle
	//TODO
		bool Braking = WheelState.DriveTorque < /*>*/FMath::Abs(WheelState.BrakeTorque);
		float BrakeFactor = 1.0f;
		float K =0.4f;
		
		// are we actually touching the ground
		if (WheelState.HitResult.bBlockingHit)
		{
		float	LongitudinalAdhesiveLimit = WheelState.WheelLoad.Size() * WheelState.HitResult.PhysMaterial.Get()->Friction * WheelState.WheelSetup->LongitudinalFrictionMultiplier;
		float	LateralAdhesiveLimit = WheelState.WheelLoad.Size() * WheelState.HitResult.PhysMaterial.Get()->Friction * WheelState.WheelSetup->LateralFrictionMultiplier;

			if (Braking)
			{

				// whether the velocity is +ve or -ve when we brake we are slowing the vehicle down
				// so force is opposing current direction of travel.
				float ForceRequiredToBringToStop = MassPerWheel * K* (GroundVelocityVector.X) / DeltaTime;
				FinalLongitudinalForce = AppliedLinearBrakeForce;

				// check we are not applying more force than required so we end up overshooting 
				// and accelerating in the opposite direction
				if (FinalLongitudinalForce > FMath::Abs(ForceRequiredToBringToStop))
				{
					FinalLongitudinalForce = FMath::Abs(ForceRequiredToBringToStop);
				
				}

				// ensure the brake opposes current direction of travel
				if (GroundVelocityVector.X > 0.0f)
				{
					FinalLongitudinalForce = -FinalLongitudinalForce;
				}

			}
			else
			{
				FinalLongitudinalForce = AppliedLinearDriveForce;
			}
			
			// lateral grip
			float FinalLateralForce = -(MassPerWheel * K * GroundVelocityVector.Y) / DeltaTime;

			
			ForceFromFriction.X = FinalLongitudinalForce;
		
			float DynamicFrictionLongitudialScaling = 0.75f;
			float TractionControlAndABSScaling = 0.98f;	// how close to perfection is the system working

			const float	SideSlipModifier = 1.0f;
				bool Locked = false;
			 WheelState.Spinning = false;
			
			// we can only obtain as much accel/decel force as the friction will allow
			if (FMath::Abs(FinalLongitudinalForce) > LongitudinalAdhesiveLimit)
			{
				if (Braking)
				{
					BrakeFactor = FMath::Clamp(LongitudinalAdhesiveLimit / FMath::Abs(FinalLongitudinalForce), 0.6f, 1.0f);
				}

				if ((Braking && WheelState.WheelSetup->ABSEnabled) || (!Braking && WheelState.WheelSetup->TractionControlEnabled))
				{
					WheelState.Spin = 0.0f;
					ForceFromFriction.X = LongitudinalAdhesiveLimit * TractionControlAndABSScaling;
				}
				else
				{
					if (!Braking)
					{
						WheelState.Spinning = true;
						WheelState.Spin += 0.5f * DeltaTime;
						WheelState.Spin = FMath::Clamp(WheelState.Spin, -2.f, 2.f);
					}
					else
					{
						Locked = true;
					}
					ForceFromFriction.X = LongitudinalAdhesiveLimit * DynamicFrictionLongitudialScaling;
					
				}
			}
			else
			{
				WheelState.Spin = 0.0f;
			}

			if (FinalLongitudinalForce < -LongitudinalAdhesiveLimit)
			{
				ForceFromFriction.X = -ForceFromFriction.X;
			}

			static float DynamicFrictionLateralScaling = 0.75f;
			if (Locked || WheelState.Spinning)
			{
				//SideSlipModifier *=1; //TODO Check thisWheelState.WheelSetup.SideSlipModifier;
			}

			// Lateral needs more grip to feel right!
			LateralAdhesiveLimit *= 1.0f * SideSlipModifier;
			ForceFromFriction.Y = FinalLateralForce;
			if (FMath::Abs(FinalLateralForce) > LateralAdhesiveLimit)
			{
				ForceFromFriction.Y = LateralAdhesiveLimit * DynamicFrictionLateralScaling;
			}

			if (FinalLateralForce < -LateralAdhesiveLimit)
			{
				ForceFromFriction.Y = -ForceFromFriction.Y;
			}


			// wheel rolling - just match the ground speed exactly
			if (BrakeFactor < 1.0f)
			{
				WheelState.Omega *= BrakeFactor;
			}
			else if (WheelState.Spin > 0.1f)
			{
				WheelState.Omega += WheelState.Spin;
			}
			else
			{
				float GroundOmega = GroundVelocityVector.X / Re;
				WheelState.Omega += (GroundOmega - WheelState.Omega);
			}

		}
		// Wheel angular position
		WheelState.AngularPosition += WheelState.Omega * DeltaTime;

		while (WheelState.AngularPosition >= PI * 2.f)
		{
			WheelState.AngularPosition -= PI * 2.f;
		}
		while (WheelState.AngularPosition <= -PI * 2.f)
		{
			WheelState.AngularPosition += PI * 2.f;
		}

		if (!WheelState.HitResult.bBlockingHit)
		{
			ForceFromFriction = FVector::ZeroVector;
			return;
		}

		
	
	

	//TODO Do we need this?float RotationAngle = FMath::RadiansToDegrees(WheelState.AngularPosition);
	FVector FrictionForceLocal = ForceFromFriction;
    FrictionForceLocal = SteeringRotator.RotateVector(FrictionForceLocal);

	FVector GroundZVector = WheelState.HitResult.ImpactNormal;
	FVector GroundXVector = FVector::CrossProduct(GetMesh()->GetRightVector(), GroundZVector);
	FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
	FMatrix Mat(GroundXVector, GroundYVector, GroundZVector, GetMesh()->GetComponentLocation());
	FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);

	GetMesh()->AddForceAtLocation(FrictionForceVector,ArcadeWheel->GetComponentLocation());
	if(GArcadeVehicleDebugParams.ShowDrawFriction)
	DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+FrictionForceVector,FColor::Green,false,-1,0,15);

}

float UArcadeMovementComponent::CmToM(float In)
{
	return  In*100;
}


float UArcadeMovementComponent::CalcSteeringInput()
{
	if(GArcadeVehicleDebugParams.ShowInputProcessingDebug)
		UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc SteeringInput: %f "),FMath::Abs(RawSteeringInput));
	return  RawSteeringInput;
}

float UArcadeMovementComponent::CalcBrakeInput()
{
	if (VehicleState.VehicleData->bReverseAsBrake)
	{
		float NewBrakeInput = 0.0f;

		// if player wants to move forwards...
		if (RawThrottleInput > 0.f)
		{
			// if vehicle is moving backwards, then press brake
			if (VehicleState.ForwardSpeed < -VehicleState.VehicleData->WrongDirectionThreshold)
			{
				NewBrakeInput = 1.0f;
			}

		}

		// if player wants to move backwards...
		else if (RawBrakeInput > 0.f)
		{
			// if vehicle is moving forwards, then press brake
			if (VehicleState.ForwardSpeed > VehicleState.VehicleData->WrongDirectionThreshold)
			{
				NewBrakeInput = 1.0f;
			}
		}
		// if player isn't pressing forward or backwards...
		else
		{
			if (VehicleState.ForwardSpeed <VehicleState.VehicleData->StopThreshold && VehicleState.ForwardSpeed > -VehicleState.VehicleData->StopThreshold)	//auto brake 
				{
				NewBrakeInput = 1.f;
				}
			else
			{
				NewBrakeInput = VehicleState.VehicleData->IdleBrakeInput;
			}
		}

		return FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
	}
	else
	{
		float NewBrakeInput = FMath::Abs(RawBrakeInput);

		// if player isn't pressing forward or backwards...
		if (RawBrakeInput < SMALL_NUMBER && RawThrottleInput < SMALL_NUMBER)
		{
			if (VehicleState.ForwardSpeed < VehicleState.VehicleData->StopThreshold && VehicleState.ForwardSpeed > -VehicleState.VehicleData->StopThreshold)	//auto brake 
				{
				NewBrakeInput = 1.f;
			
				}
		}

		if(GArcadeVehicleDebugParams.ShowInputProcessingDebug)
		UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc NewBrakeInput: %f "),FMath::Abs(NewBrakeInput));
		return NewBrakeInput;
	}

}
float UArcadeMovementComponent::CalcHandbrakeInput()
{
	return (bRawHandbrakeInput == true) ? 1.0f : 0.0f;
}
float UArcadeMovementComponent::CalcThrottleInput()
{
	

	float NewThrottleInput = RawThrottleInput;
	if (VehicleState.VehicleData->bReverseAsBrake )
	{
		if (RawBrakeInput > 0.f &&VehicleState.TargetGear < VehicleState.IdleGear /*PVehicle->GetTransmission().GetTargetGear() < 0/*ForwardSpeed < -WrongDirectionThreshold*/)
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
			//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if ((RawThrottleInput > 0.f && VehicleState.TargetGear < VehicleState.IdleGear) || (RawBrakeInput > 0.f && VehicleState.TargetGear > VehicleState.IdleGear))
			{
				NewThrottleInput = 0.f;
			}
	}

	//Debug
if(GArcadeVehicleDebugParams.ShowInputProcessingDebug)
{
	UE_LOG(LogArcadeVehicle,Warning,TEXT("Throttle raw before process %f"),RawThrottleInput);
	UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc Throttle %f "),FMath::Abs(NewThrottleInput));
}
	//
	
	return FMath::Abs(NewThrottleInput);
}

void UArcadeMovementComponent::SetTargetGear(int32 GearNum, bool bImmediate)
{
	if(GArcadeVehicleDebugParams.ShowGearboxLog)
	UE_LOG(LogArcadeVehicle,Warning,TEXT("Change gear called with %d "),GearNum);
	if(VehicleState.VehicleData->Gears.IsValidIndex(GearNum))
	{
		if(bImmediate)
		{
			VehicleState.CurrentGear=VehicleState.TargetGear=GearNum;
		}else
		{
			VehicleState.TargetGear=GearNum;
			VehicleState.CurrentGearChangeTime=VehicleState.VehicleData->GearChangeTime;
		}
	}
}

////////////////
//////////////
//Debug
///////////
///////////////
void UArcadeMovementComponent::ShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo,
    float& YL, float& YPos)
{
	
	if (Canvas && HUD)
	{
		static FName NAME_Vehicle = FName(TEXT("Vehicle"));
		
		if(GetPawnOwner())
		{
			if(APlayerController* PlayerController= Cast<APlayerController>(GetPawnOwner()->GetController()))
			{
				
				if (PlayerController->IsLocalPlayerController()&& HUD->ShouldDisplayDebug(NAME_Vehicle))
				{
					
					DrawDebug(Canvas, YL, YPos);
				}
			}
			
		}
		
		
	}
}

void UArcadeMovementComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	UFont* RenderFont = GEngine->GetLargeFont();
	float X, Y;
	Canvas->GetCenter(X, Y);
	float YLine = Y * 2.f - 50.f;
	float Scaling = 2.f;
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d KMH"), (int)(VehicleState.ForwardSpeed*0.036)), X-100, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("[%d]"), (int)VehicleState.CurrentGear), X, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d RPM"), (int)VehicleState.CurrentRpm), X+50, YLine, Scaling, Scaling);
	FVector2D DialPos(X+10, YLine-40);
	float DialRadius = 50;
	DrawDial(Canvas, DialPos, DialRadius, VehicleState.CurrentRpmRatio, 1);
	
}



#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

float UArcadeMovementComponent::CalcDialAngle(float CurrentValue, float MaxValue)
{
	return (CurrentValue / MaxValue) * 3.f / 2.f * PI - (PI * 0.25f);
}

void UArcadeMovementComponent::DrawDial(UCanvas* Canvas, FVector2D Pos, float Radius, float CurrentValue, float MaxValue)
{
	float Angle = CalcDialAngle(CurrentValue, MaxValue);
	FVector2D PtEnd(Pos.X - FMath::Cos(Angle) * Radius, Pos.Y - FMath::Sin(Angle) * Radius);
	DrawLine2D(Canvas, Pos, PtEnd, FColor::White, 3.f);

	for (float I = 0; I < MaxValue; I += 1000.0f)
	{
		Angle = CalcDialAngle(I, MaxValue);
		PtEnd.Set(-FMath::Cos(Angle) * Radius, -FMath::Sin(Angle) * Radius);
		FVector2D PtStart = PtEnd * 0.8f;
		DrawLine2D(Canvas, Pos + PtStart, Pos + PtEnd, FColor::White, 2.f);
	}

	// the last checkmark
	Angle = CalcDialAngle(MaxValue, MaxValue);
	PtEnd.Set(-FMath::Cos(Angle) * Radius, -FMath::Sin(Angle) * Radius);
	FVector2D PtStart = PtEnd * 0.8f;
	DrawLine2D(Canvas, Pos+PtStart, Pos+PtEnd, FColor::Red, 2.f);

}


void UArcadeMovementComponent::DrawLine2D(UCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, FColor Color, float Thickness)
{
	if (Canvas)
	{
		FCanvasLineItem LineItem(StartPos, EndPos);
		LineItem.SetColor(Color);
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}
}


#endif




#undef LOCTEXT_NAMESPACE