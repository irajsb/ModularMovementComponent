// Fill out your copyright notice in the Description page of Project Settings.
//TODO Replicate cosmetic data
//TODO Audio
//TODO Pathfinding
//TODO Avoidance
//TODO SkeletalMesh
//TODO sliding
//TODO fix gearbox
//Cosmetic delegates

#include "ModularMovementComponent.h"

#include "ArcadePawn.h"
#include "WheelInterface.h"
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

UModularMovementComponent::UModularMovementComponent()
{
	AHUD::OnShowDebugInfo.AddUObject(this, &UModularMovementComponent::ShowDebugInfo);
	SetIsReplicatedByDefault(true);
}

UMeshComponent* UModularMovementComponent::GetMesh()const
{
	return  Cast<UMeshComponent>(UpdatedComponent);
}

void UModularMovementComponent::SetThrottleInput(float Input)
{
	RawThrottleInput=FMath::Clamp<float>(Input,-1.f,1.f);
}

void UModularMovementComponent::SetSteeringInput(float Input)
{
	RawSteeringInput=FMath::Clamp<float>(Input,-1.f,1.f);
}

void UModularMovementComponent::SetBrakeInput(float Brake)
{
	RawBrakeInput = FMath::Clamp(Brake, -1.0f, 1.0f);
}

void UModularMovementComponent::SetHandBrakeInput(bool Brake)
{
	HandBrakeInput=Brake;
}

int UModularMovementComponent::GetNumberOfWheels()
{//TODo
	return 4;
}

FArcadeGearInfo UModularMovementComponent::GetGearInfo(int Index)
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

void UModularMovementComponent::InitializeComponent()
{


	Super::InitializeComponent();
if(!VehicleState.VehicleData)
{
	UE_LOG(LogArcadeVehicle,Error,TEXT("Assign The Vehicle DataAsset "));
	FMessageLog("Blueprint").Error(LOCTEXT("VehicleDataAssetNotValid", "Passed Assign Vehicle Data Asset "));
	return;
}
	
	Components=	GetOwner()->GetComponentsByInterface(UWheelInterface::StaticClass());
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

	//Setting up Wheel initial Data
	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->SetupWheels(this);
	}
	
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
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
	if(ShouldProcessInput())
	{
		UE_LOG(LogTemp,Log,TEXT("%s is processing input"),*GetOwner()->GetHumanReadableName());
		//code from Prv Vehicle
		const int32 QThrottleInput = FMath::FloorToInt(RawThrottleInput * 127.f) & 0xFF;
		const int32 QSteeringInput = (FMath::FloorToInt(RawSteeringInput * 63.f) & 0x7F) << 8;
		const int32 QHandbrakeInput = HandBrakeInput ? (1 << 15) : 0;
		const uint16 NewQuantizeInput = QHandbrakeInput | QSteeringInput | QThrottleInput;

		if (QuantizeInput != NewQuantizeInput)
		{
			QuantizeInput = NewQuantizeInput;
			ServerUpdateState(QuantizeInput);
		}
	}
	
	if(ShouldProcessPhysics())
	{
		UE_LOG(LogTemp,Log,TEXT("%s is processing physics delta : %f "),*GetOwner()->GetHumanReadableName(),fDeltaTime);
	
		UpdateEngine(fDeltaTime);
		UpdateSuspension(fDeltaTime);
		UpdateSteering(fDeltaTime);
		UpdateGearBox(fDeltaTime);
		UpdateForces(fDeltaTime);
	}
	if(ShouldProcessCosmetics())
	{
		UE_LOG(LogTemp,Log,TEXT("%s is processing Cosmetics"),*GetOwner()->GetHumanReadableName());
		if(GetOwnerRole()!=ENetRole::ROLE_Authority)
		{
			SimulateWheelData(fDeltaTime);
		}
		UpdateWheelAnimation(DeltaTime);
	}
}

void UModularMovementComponent::UpdateState(float DeltaTime)
{
	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetPhysicsLinearVelocity(), GetMesh()->GetForwardVector());
}

void UModularMovementComponent::UpdateGearBox(float DeltaTime)
{

	if (VehicleState.VehicleData->bReverseAsBrake)
	{
		//for reverse as state we want to automatically shift between reverse and first gear
		if (FMath::Abs(VehicleState.ForwardSpeed) < VehicleState.VehicleData->WrongDirectionThreshold)	//we only shift between reverse and first if the car is slow enough.
			{
			if (RawThrottleInput < -1*KINDA_SMALL_NUMBER &&VehicleState.CurrentGear >= VehicleState.IdleGear && VehicleState.TargetGear >= VehicleState.IdleGear)
			{
				SetTargetGear(VehicleState.IdleGear-1, true);
			}
			else if (RawThrottleInput > KINDA_SMALL_NUMBER &&VehicleState.CurrentGear <= VehicleState.IdleGear && VehicleState.TargetGear <= VehicleState.IdleGear)
			{
				SetTargetGear(VehicleState.IdleGear+1, true);
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
			SetTargetGear(VehicleState.CurrentGear-1,true);
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

void UModularMovementComponent::UpdateEngine(float DeltaTime)
{

	ARCADE_CYCLE_COUNTER(STAT_ArcadeEngine)
	float HighestOmega=0;
	for(UActorComponent* Component: Components)
	{
	
	const float ComponentOmega=	FMath::Abs(Cast<IWheelInterface>(Component)->GetFastestWheelOmegaSpeed());
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
		Cast<IWheelInterface>(Component)->SetDriveTorqueOnWheels(WheelTorque);
	}
	
}



void UModularMovementComponent::UpdateSuspension(float DeltaTime)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeSuspension)

	

	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->UpdateSuspension(DeltaTime,this);
	}
}

void UModularMovementComponent::UpdateForces(float DeltaTime)
{

	
	ARCADE_CYCLE_COUNTER(STAT_ArcadeForces)


	 BrakeInput=CalcBrakeInput();
	if(GArcadeVehicleDebugParams.ShowInputProcessingDebug)
		UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc NewBrakeInput: %f "),FMath::Abs(BrakeInput));

	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->UpdateForces(DeltaTime,this);
	}

}

void UModularMovementComponent::UpdateSteering(float DeltaTime)
{

	SteeringInput=CalcSteeringInput(DeltaTime);

	const float SteerSpeedScale =	VehicleState.VehicleData->SteerCurve.GetRichCurve()->IsEmpty()?1:VehicleState.VehicleData->SteerCurve.GetRichCurve()->Eval(VehicleState.ForwardSpeed*0.036/*CmSToKmH*/) ;
	
const 	float UseSteeringValue = SteeringInput * SteerSpeedScale;
	

	
	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->UpdateSteering(DeltaTime,this,UseSteeringValue);
	}
	
}

void UModularMovementComponent::UpdateWheelAnimation(float DeltaTime)
{

for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->UpdateAnimation(DeltaTime,this);
	}
}

void UModularMovementComponent::SimulateWheelData(float DeltaTime)
{
	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->SimulateWheelData(DeltaTime,this);
	}
}

void UModularMovementComponent::CalculateSteeringAngle(FWheelState& WheelState, float DeltaTime, USceneComponent* ArcadeWheel,float InNormSteering) const
{
	
	/*if (FMath::Abs(GWheeledVehicleDebugParams.SteeringOverride) > 0.01f)
	{
	SteeringAngle = PWheel.Setup().WheelState.WheelSetup->SteeringMaxAngle * GWheeledVehicleDebugParams.SteeringOverride;
	}
	else*/
	{
		//

		const float WheelSide =WheelState.InitialLocalLocation.Y;
		


		//GetSteeringAngleChaos


		float OutSteeringAngle = 0.f;

		switch (VehicleState.VehicleData->SteerType)
		{
		case EArcadeSteerType::AngleRatio:
			{
				const bool OutsideWheel = (InNormSteering * WheelSide) > 0.f;
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



void UModularMovementComponent::WheelTrace(
                                          FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel) const
{

	//logic
	TArray<AActor*> ActorsToIgnore;
	WheelState.WheelLoad=FVector::ZeroVector;
	ActorsToIgnore.Add(GetOwner());
	//TODO Generalize it
	const FVector ComponentLocation=GetMesh()->GetComponentTransform().TransformPosition(WheelState.InitialLocalLocation+WheelState.WheelSetup->TraceStartOffset) ;
	FHitResult TraceResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(),ComponentLocation,ComponentLocation+(GetMesh()->GetUpVector()*-1*WheelState.WheelSetup->SuspensionLength),WheelState.WheelSetup->WheelRadius,VehicleState.VehicleData->SuspensionTraceTypeQuery,true,ActorsToIgnore,GArcadeVehicleDebugParams.ShowSuspensionDebug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,TraceResult,true);

	const float CurrentLen=1-TraceResult.Time;
	const float DampingCorrection=(((CurrentLen-WheelState.PreviousLen)*VehicleState.VehicleData->DampingCorrectionMultiplier*WheelState.WheelSetup->Stiffness))/DeltaTime;
	if(TraceResult.bBlockingHit&&GetOwnerRole()==ENetRole::ROLE_Authority)
	{
	WheelState.WheelLoad=DeltaTime*((FVector(0,0,1)*(WheelState.WheelSetup->Stiffness+DampingCorrection)*(CurrentLen)));
	GetMesh()->AddForceAtLocation(WheelState.WheelLoad,TraceResult.TraceStart);
	
	}
	
	WheelState.PreviousLen=CurrentLen;
	WheelState.HitResult=TraceResult;

	
	//logic
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(GArcadeVehicleDebugParams.ShowSuspensionDebug)
	{
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+(DeltaTime*(FVector(0,0,1)*(WheelState.WheelSetup->Stiffness/1000)*(CurrentLen))),FColor::Red,false,-1,0,5);
	

	}


	#endif

}


void UModularMovementComponent::SimulateWheel(FWheelState& WheelState, float DeltaTime,
    USceneComponent* ArcadeWheel)
{

//maybe you'll need to make a simplified version

SteeringInput=CalcSteeringInput(DeltaTime);
const float SteerSpeedScale =	VehicleState.VehicleData->SteerCurve.GetRichCurve()->IsEmpty()?1:VehicleState.VehicleData->SteerCurve.GetRichCurve()->Eval(VehicleState.ForwardSpeed*0.036/*CmSToKmH*/) ;	
const 	float UseSteeringValue = SteeringInput * SteerSpeedScale;
WheelTrace(WheelState,DeltaTime,ArcadeWheel);
ApplyWheelForces(WheelState,DeltaTime,ArcadeWheel);
if(WheelState.WheelSetup->SteeringWheel)
CalculateSteeringAngle(WheelState,DeltaTime,ArcadeWheel,UseSteeringValue);


}
void UModularMovementComponent::ApplyWheelForces(FWheelState& WheelState, float DeltaTime,
    USceneComponent* ArcadeWheel)
{
	
//calculate Brake
	
	const float BrakeTorque=BrakeInput*WheelState.WheelSetup->BrakeTorque;
	WheelState.BrakeTorque=BrakeTorque;
	if(	HandBrakeInput)
	{
		if(WheelState.WheelSetup->AffectedByHandBrake)
		{
			WheelState.BrakeTorque=WheelState.WheelSetup->HandBrakeTorque;
			WheelState.DriveTorque=0;
		}
	}
	
	
	const FTransform WorldTransform = GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = WheelState.SteerAngle; // temp
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	//TODO Generalize it 
	const FVector  LocalWheelVelocity = WorldTransform.InverseTransformVector(GetMesh()->GetPhysicsLinearVelocityAtPoint(WheelState.HitResult.TraceStart));
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	//TODo whats this ?const float SlipAngle = FMath::Atan2(GroundVelocityVector.Y, GroundVelocityVector.X);
	float FinalLongitudinalForce = 0.f;
	FVector ForceFromFriction;
	//EffectiveRadius
	const float Re=WheelState.WheelSetup->WheelRadius;
	const float MassPerWheel=(GetMesh()->GetMass()/GetNumberOfWheels());
	float 	AppliedLinearDriveForce = WheelState.DriveTorque * CmToM(Re);
	
	float AppliedLinearBrakeForce = WheelState.BrakeTorque* CmToM(Re);

		// currently just letting the brake override the throttle


		bool Braking = FMath::Abs(WheelState.DriveTorque) < /*>*/FMath::Abs(WheelState.BrakeTorque);
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
		
			float DynamicFrictionLongitudinalScaling = 0.75f;
			float TractionControlAndAbsScaling = 0.98f;	// how close to perfection is the system working

			const float	SideSlipModifier = 1.0f;
				bool Locked = false;
			 WheelState.Spinning = false;
			
			// we can only obtain as much accel force as the friction will allow
			if (FMath::Abs(FinalLongitudinalForce) > LongitudinalAdhesiveLimit)
			{
				if (Braking)
				{
					BrakeFactor = FMath::Clamp(LongitudinalAdhesiveLimit / FMath::Abs(FinalLongitudinalForce), 0.6f, 1.0f);
				}

				if ((Braking && WheelState.WheelSetup->ABSEnabled) || (!Braking && WheelState.WheelSetup->TractionControlEnabled))
				{
					WheelState.Spin = 0.0f;
					ForceFromFriction.X = LongitudinalAdhesiveLimit * TractionControlAndAbsScaling;
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
					ForceFromFriction.X = LongitudinalAdhesiveLimit * DynamicFrictionLongitudinalScaling;
					
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

		
	
	

	if(!WheelState.HitResult.bBlockingHit)
		return;
	//TODO Do we need this?float RotationAngle = FMath::RadiansToDegrees(WheelState.AngularPosition);
	if(GetOwnerRole()==ENetRole::ROLE_Authority)
	{
		FVector FrictionForceLocal = ForceFromFriction;
		FrictionForceLocal = SteeringRotator.RotateVector(FrictionForceLocal);

		FVector GroundZVector = WheelState.HitResult.ImpactNormal;
		FVector GroundXVector = FVector::CrossProduct(GetMesh()->GetRightVector(), GroundZVector);
		FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
		FMatrix Mat(GroundXVector, GroundYVector, GroundZVector, GetMesh()->GetComponentLocation());
		FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);
	
		GetMesh()->AddForceAtLocation(FrictionForceVector,WheelState.HitResult.TraceStart);
		if(GArcadeVehicleDebugParams.ShowDrawFriction)
			DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+FrictionForceVector,FColor::Green,false,-1,0,15);
	}
}

float UModularMovementComponent::CmToM(float In)
{
	return  In*100;
}



bool UModularMovementComponent::ShouldProcessPhysics()const
{
	return GetOwnerRole()==ENetRole::ROLE_Authority;
}

bool UModularMovementComponent::ShouldProcessCosmetics()const
{
return 	GetNetMode()==ENetMode::NM_Standalone||(GetNetMode()==ENetMode::NM_Client&&GetOwnerRole()!=ENetRole::ROLE_Authority);
}

bool UModularMovementComponent::ShouldProcessInput()const
{
	return 	(GetPawnOwner()->GetLocalRole()!=ENetRole::ROLE_Authority&&GetPawnOwner()->IsLocallyControlled());
}

void UModularMovementComponent::ServerUpdateState_Implementation(uint16 InQuantizeInput)
{
	
	const int32 QThrottleInput = static_cast<int8>(InQuantizeInput & 0xFF);
	const int32 QSteeringInput = static_cast<int8>(((InQuantizeInput >> 8) & 0x7F) << 1) / 2;
	const int32 QHandbrakeInput = (InQuantizeInput >> 15) & 1;

	SetThrottleInput(QThrottleInput / 127.f);
	SetSteeringInput(QSteeringInput / 63.f);
	UE_LOG(LogTemp,Error,TEXT("Handbrake is %d"),QHandbrakeInput);
	//HandBrakeInput = QHandbrakeInput;

	//LastUserSteeringInput = QSteeringInput;
}


float UModularMovementComponent::CalcSteeringInput(float DeltaTime)
{
	

SteeringInput=	UKismetMathLibrary::FInterpTo_Constant(SteeringInput,RawSteeringInput,DeltaTime,VehicleState.VehicleData->SteeringInterpolationSpeed);
	if(FMath::Abs(SteeringInput)>FMath::Abs(RawSteeringInput))
	{//decreases are instant
		SteeringInput=RawSteeringInput;
	}
	if(GArcadeVehicleDebugParams.ShowInputProcessingDebug)
		UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc SteeringInput: %f "),(SteeringInput));
	return  SteeringInput;
}

float UModularMovementComponent::CalcBrakeInput()const
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
		else if (RawThrottleInput < 0.f)
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

		
		return NewBrakeInput;
	}

}

float UModularMovementComponent::CalcThrottleInput()
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
	
	//Why??return FMath::Abs(NewThrottleInput);
	return NewThrottleInput;
}

void UModularMovementComponent::SetTargetGear(int32 GearNum, bool bImmediate)
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
void UModularMovementComponent::ShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo,
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

void UModularMovementComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	UFont* RenderFont = GEngine->GetLargeFont();
	float X, Y;
	Canvas->GetCenter(X, Y);
	const float YLine = Y * 2.f - 50.f;
	const float Scaling = 2.f;
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d KMH"),static_cast<int>(VehicleState.ForwardSpeed * 0.036)), X-150, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("[%d]"), static_cast<int>(VehicleState.CurrentGear)), X, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d RPM"),static_cast<int>(VehicleState.CurrentRpm)), X+50, YLine, Scaling, Scaling);
	const FVector2D DialPos(X+10, YLine-40);
const	float DialRadius = 50;
	DrawDial(Canvas, DialPos, DialRadius, VehicleState.CurrentRpmRatio, 1);
	
}



#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

float UModularMovementComponent::CalcDialAngle(float CurrentValue, float MaxValue)
{
	return (CurrentValue / MaxValue) * 3.f / 2.f * PI - (PI * 0.25f);
}

void UModularMovementComponent::DrawDial(UCanvas* Canvas, FVector2D Pos, float Radius, float CurrentValue, float MaxValue)
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
	const FVector2D PtStart = PtEnd * 0.8f;
	DrawLine2D(Canvas, Pos+PtStart, Pos+PtEnd, FColor::Red, 2.f);

}


void UModularMovementComponent::DrawLine2D(UCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, FColor Color, float Thickness)
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