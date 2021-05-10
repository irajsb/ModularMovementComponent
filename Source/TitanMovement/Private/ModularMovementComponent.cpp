// Fill out your copyright notice in the Description page of Project Settings.
//TODO TankSteering

//TODO Pathfinding
//TODO Avoidance
//TODO SkeletalMesh
//TODO sliding
//TODO fix gearbox


#include "ModularMovementComponent.h"


#include "WheelInterface.h"
#include "TitanMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
#include "ModularVehicleFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsSettings.h"
FModularVehicleDebugParams GModularVehicleDebugParams;

DECLARE_CYCLE_STAT(TEXT("Arcade Tick Component"), STAT_ArcadeTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Engine"), STAT_ArcadeEngine, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Suspension"), STAT_ArcadeSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Forces"), STAT_ArcadeForces, STATGROUP_MovementPhysics);


static FAutoConsoleVariableRef CVarArcadeVehicleShowSuspensionDebug(
    TEXT("ModularVehicle.ShowSuspensionDebug"),
    GModularVehicleDebugParams.ShowSuspensionDebug,
    TEXT("Toggles Suspension Debugging visuals"));
static FAutoConsoleVariableRef CVarArcadeVehicleShowInputProcessLog(
    TEXT("ModularVehicle.ShowInputProcessLog"),
    GModularVehicleDebugParams.ShowInputProcessingDebug,
    TEXT("Toggles Input Debugging UE_LOGs"));

static FAutoConsoleVariableRef CVarArcadeVehicleShowGearBoxProcessLog(
    TEXT("ModularVehicle.ShowGearBoxProcessLog"),
    GModularVehicleDebugParams.ShowGearboxLog,
    TEXT("Toggles GearBox Debugging UE_LOGs"));
static FAutoConsoleVariableRef CVarArcadeVehicleFrictionDraw(
    TEXT("ModularVehicle.ShowFriction"),
    GModularVehicleDebugParams.ShowDrawFriction,
    TEXT("Toggles Friction force "));
static FAutoConsoleVariableRef CVarArcadeVehicleAIDebug(
    TEXT("ModularVehicle.AIDebug"),
    GModularVehicleDebugParams.AIDebug,
    TEXT("Toggles AI Debug "));

#define LOCTEXT_NAMESPACE "ArcadeMovement"

FORCEINLINE float OmegaToRPM(float Omega)
{
	return Omega * 30.f / PI;
}

UModularMovementComponent::UModularMovementComponent()
{
	AHUD::OnShowDebugInfo.AddUObject(this, &UModularMovementComponent::ShowDebugInfo);
	SetIsReplicated(true);
	
}

void UModularMovementComponent::UpdateComponents()
{
	TArray<UActorComponent*> TempComponents;

	if (UWheelInterface::StaticClass())
	{
		for (UActorComponent* Component :GetOwner()->GetComponents())
		{
			if (Component && Component->GetClass()->ImplementsInterface(UWheelInterface::StaticClass()))
			{
				TempComponents.Add(Component);
			}
		}
	}

Components=TempComponents;
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

	return;
}
	UpdateNavAgent(*GetOwner());
	UpdateComponents();
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

	//allow wheels to init  the variables that they need
	for(UActorComponent* Component: Components)
	{
		Cast<IWheelInterface>(Component)->SetupWheels(this);
	
	}
	
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ARCADE_CYCLE_COUNTER(STAT_ArcadeTickComponent)
	const float fDeltaTime=FMath::Min<float>(DeltaTime,0.0333);
	if(!VehicleState.VehicleData)
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("Assign The Vehicle DataAsset "));
	
		return;
	}
	
	UpdateState(fDeltaTime);
	if(ShouldProcessInput())
	{
		
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
		

		UpdateSteering(fDeltaTime);
		UpdateEngine(fDeltaTime);
		UpdateSuspension(fDeltaTime);
		UpdateGearBox(fDeltaTime);
		UpdateForces(fDeltaTime);
		UpdateReplicatedCosmeticData();
	}
	if(ShouldProcessCosmetics())
	{
		
		if(GetOwnerRole()!=ENetRole::ROLE_Authority)
		{
			SimulateWheelData(fDeltaTime);
		}
		UpdateWheelAnimation(DeltaTime);
	}
}

void UModularMovementComponent::UpdateState(float DeltaTime)
{
	VehicleState.DriveWheelsOnGround=0;
	for(UActorComponent* Component: Components)
	{
		VehicleState.DriveWheelsOnGround+=Cast<IWheelInterface>(Component)->GetNumOfWheelsTouchingGround(true);
	}
	
	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetPhysicsLinearVelocity(), GetMesh()->GetForwardVector());
}

void UModularMovementComponent::UpdateGearBox(float DeltaTime)
{
if(VehicleState.DriveWheelsOnGround!=0)
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
}
	if (VehicleState.CurrentGear != VehicleState.TargetGear)
	{
		VehicleState.CurrentGearChangeTime -= DeltaTime;
		if (VehicleState.CurrentGearChangeTime <= 0.f)
		{
			VehicleState.CurrentGearChangeTime = 0.f;
			OnGearChange.Broadcast(VehicleState.CurrentGear,VehicleState.TargetGear,true);
			VehicleState.CurrentGear =VehicleState.TargetGear;
			if(GModularVehicleDebugParams.ShowGearboxLog)
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
	
	if(VehicleState.DriveWheelsOnGround==0)
	{
		VehicleState.CurrentRpm=FMath::Clamp<float>((VehicleState.VehicleData->MaxRpm*ThrottleInput),VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
	}
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
	if(GModularVehicleDebugParams.ShowInputProcessingDebug)
	{
		UE_LOG(LogArcadeVehicle,Warning,TEXT("Final Calc NewBrakeInput: %f "),FMath::Abs(BrakeInput));
	
	}

	
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

EAIVehicleState UModularMovementComponent::DetermineAIState(float ForwardFactor,float DeltaTime)
{

	UWorld* World=GetWorld();
	const FVector VehicleLocation = GetOwner()->GetActorLocation();
	FHitResult  HitResultF;
	FHitResult  HitResultB;
	const FVector VehicleDirection=GetMesh()->GetForwardVector();

	//trace
		if(World)
		{
		TArray<AActor*> ActorsToIgnore;
	
		ActorsToIgnore.Add(GetOwner());
		
		
		const float TraceLenForward=VehicleState.VehicleData->TraceLength*FMath::Max(VehicleState.VehicleData->TraceLength,static_cast<float>((VehicleState.ForwardSpeed*0.036*VehicleState.VehicleData->TraceSpeedMultiplier)));
		const float TraceLenBackWard=VehicleState.VehicleData->TraceLength*FMath::Max(VehicleState.VehicleData->TraceLength,static_cast<float>((-1*VehicleState.ForwardSpeed*0.036*VehicleState.VehicleData->TraceSpeedMultiplier)));
		//start traces
		
		UKismetSystemLibrary::LineTraceSingle(World,VehicleLocation,VehicleLocation+VehicleDirection*TraceLenForward,VehicleState.VehicleData->SuspensionTraceTypeQuery,false,ActorsToIgnore,GModularVehicleDebugParams.AIDebug?EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,HitResultF,true);
		UKismetSystemLibrary::LineTraceSingle(World,VehicleLocation,VehicleLocation+-1*VehicleDirection*TraceLenBackWard,VehicleState.VehicleData->SuspensionTraceTypeQuery,false,ActorsToIgnore,GModularVehicleDebugParams.AIDebug?EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,HitResultB,true);

		}
	//determine state
	//turn around state

VehicleState.LockCurrentStateDelta-=DeltaTime;

	EAIVehicleState OutState=VehicleState.AIState;
	//if state is locked but there is crash risk avoid it
	if(VehicleState.LockCurrentStateDelta>0)
	{
		if(VehicleState.AIState==TurningAround&&HitResultB.bBlockingHit)
		{
			OutState=Normal;
		}
		if(VehicleState.AIState!=TurningAround&&HitResultF.bBlockingHit)
			{
			OutState=TurningAround;
			}
		return  OutState;
	}
	//state not locked
	//Normally moving forward
	
	if (VehicleState.AIState==Normal)
	{
		//if We need to turn or turn around
		if(ForwardFactor<VehicleState.VehicleData->ReverseThreshold)
		{
			OutState=TurningAround;
		}
		//forward is  blocked
		if(!HitResultB.bBlockingHit&&HitResultF.bBlockingHit)
		{
			OutState=TurningAround;
			VehicleState.LockCurrentStateDelta=0.5;
		}


		
	}

	//Turning Around
	else if(VehicleState.AIState==TurningAround)
	{
		if(FMath::IsNearlyEqual(ForwardFactor,1.0f,0.1f))
		{
			OutState=Normal;
		}
		//if back is blocked forward is open we stop reversing
		if(HitResultB.bBlockingHit&&!HitResultF.bBlockingHit)
		{
			VehicleState.LockCurrentStateDelta=0.5;
			OutState=Normal;
		}
	}
	

	return OutState;
}


void UModularMovementComponent::CalculateSteeringAngle(FWheelState& WheelState, float DeltaTime, USceneComponent* ArcadeWheel,float InNormSteering) 
{


	
	const float AISteerMultiplier=VehicleState.IsAIVehicle?VehicleState.VehicleData->AIMaxSteerMultiplier:1;
	/*if (FMath::Abs(GWheeledVehicleDebugParams.SteeringOverride) > 0.01f)
	{
	SteeringAngle = PWheel.Setup().WheelState.WheelSetup->SteeringMaxAngle * GWheeledVehicleDebugParams.SteeringOverride;
	}
	else*/
	{
		//

		const float WheelSide =WheelState.InitialLocalLocation.Y;
		
		float OutSteeringAngle = 0.f;

		switch (VehicleState.VehicleData->SteerType)
		{
		case EArcadeSteerType::AngleRatio:
			{
				const bool OutsideWheel = (InNormSteering * WheelSide) > 0.f;
				OutSteeringAngle = InNormSteering * (OutsideWheel ? WheelState.WheelSetup->SteeringMaxAngle*AISteerMultiplier : WheelState.WheelSetup->SteeringMaxAngle*AISteerMultiplier *0.7/*TODO Setup().AngleRatio*/);

					
			}
			break;

		case EArcadeSteerType::Tank:
			{
				
				const float LeftTrackInput=InNormSteering;
				const float RightTrackInput=-InNormSteering;
				 VehicleState.TrackLeft.TorqueTransfer=0;
				 VehicleState.TrackRight.TorqueTransfer=0;
				if (FMath::Abs(RawThrottleInput) > SMALL_NUMBER)
				{
					VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(RawThrottleInput)  + LeftTrackInput ;
					VehicleState.TrackRight.TorqueTransfer = FMath::Abs(RawThrottleInput)  +RightTrackInput ;
				}
				else
				{
				
					VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(RawThrottleInput)  + LeftTrackInput ;
					VehicleState.TrackRight.TorqueTransfer = FMath::Abs(RawThrottleInput)  + RightTrackInput ;
					
				}
				
				if(WheelSide>0)
				{
					WheelState.TorqueTransferFactor=VehicleState.TrackRight.TorqueTransfer;
				}else
				{
					WheelState.TorqueTransferFactor=VehicleState.TrackLeft.TorqueTransfer;
					
				}OutSteeringAngle=0;
				if(GModularVehicleDebugParams.ShowInputProcessingDebug)
				{
					UE_LOG(LogArcadeVehicle,Warning,TEXT("Tank input Left %f Right %f"),VehicleState.TrackLeft.TorqueTransfer,VehicleState.TrackRight.TorqueTransfer);
				}
				}
			break;

		default:
        case EArcadeSteerType::SingleAngle:
			{
				
				OutSteeringAngle = WheelState.WheelSetup->SteeringMaxAngle * InNormSteering*AISteerMultiplier;
			}
			break;

		}

		
		//
		WheelState.SteerAngle=OutSteeringAngle;
	
	}

	
	
}

void UModularMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	Super::RequestDirectMove(MoveVelocity, bForceMaxSpeed);

	VehicleState.IsAIVehicle=true;
	 VehicleState.DesiredSpeed=0.0f;
	const float DeltaSeconds=GetWorld()->GetDeltaSeconds();
	const FVector VehicleLocation = GetOwner()->GetActorLocation();
	const	FVector Destination = VehicleLocation + MoveVelocity * DeltaSeconds;
	const FVector Distance = Destination - VehicleLocation;
	const FVector VehicleDirection=GetMesh()->GetForwardVector();
	float ForwardFactor = FVector::DotProduct(VehicleDirection, Distance.GetSafeNormal());
	
	
	
	
	float CurrentYaw = Distance.Rotation().Yaw - GetMesh()->GetForwardVector().Rotation().Yaw;
	if (CurrentYaw < -180)
	{
		CurrentYaw += 360;
	}
	else if (CurrentYaw > 180)
	{
		CurrentYaw -= 360;
	}

	float SteeringPosition =(1-( (-CurrentYaw + 180) / 180))*10;
	
	VehicleState.AIState=DetermineAIState(ForwardFactor,DeltaSeconds);
	
	
	//react to state

	switch (VehicleState.AIState)
	{
	case EAIVehicleState::Normal:
		SetSteeringInput(SteeringPosition);
		VehicleState.DesiredSpeed=UKismetMathLibrary::MapRangeClamped(Distance.Size(),0,VehicleState.VehicleData->NearGoalDistance,VehicleState.VehicleData->DesireSpeedNearGoal,VehicleState.VehicleData->DesireSpeedNormal);
		if(ForwardFactor<VehicleState.VehicleData->TurnThreshold)
		{
			VehicleState.DesiredSpeed=VehicleState.VehicleData->DesireSpeedTurning;
		}
		break;
	case EAIVehicleState::TurningAround:
		VehicleState.DesiredSpeed=VehicleState.VehicleData->DesireSpeedTurningAround;
		SetSteeringInput(SteeringPosition>0 ?-1:1);
	
		break;

	}


	//match desired speed
	const float SpeedDifference =VehicleState.DesiredSpeed-VehicleState.ForwardSpeed * 0.036;//cms to kmh


	
	
	if(SpeedDifference>0)
	{//we should add throttle
	SetThrottleInput(FMath::Clamp<float>(SpeedDifference/VehicleState.VehicleData->FullThrottleSpeed,-1,1));
	}else
	{//brake or release throttle 
		SetThrottleInput(FMath::Clamp<float>(SpeedDifference/VehicleState.VehicleData->FullThrottleSpeed,-1,1));
	}
	
	if(VehicleState.ForwardSpeed)
	if(GModularVehicleDebugParams.AIDebug)
	{
		//draw destination
		DrawDebugSphere(GetWorld(),Destination,30,20,FColor::Red);
		FString VehicleStateString;
		switch (VehicleState.AIState)
		{
		case EAIVehicleState::Normal:
		VehicleStateString=TEXT("Normal");
			break;
		case EAIVehicleState::TurningAround:
			VehicleStateString=TEXT("TurningAround");
			break;
		
			
		}
	
		UE_LOG(LogArcadeVehicle,Log,TEXT("ForwardFactor= %f Distance*velocity= %f Steering %f State: %s DesiredSpeed: %f "),ForwardFactor,Distance.Size()*VehicleState.ForwardSpeed,SteeringPosition,*VehicleStateString,VehicleState.DesiredSpeed);
		
	}
	VehicleState.AIPreviousThrottle=RawThrottleInput;
}


void UModularMovementComponent::WheelTrace(
                                          FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel) const
{

	//logic
	TArray<AActor*> ActorsToIgnore;
	WheelState.WheelLoad=FVector::ZeroVector;
	ActorsToIgnore.Add(GetOwner());
	//TODO This will not work with some meshes but there is no need to change for now 
	const FVector ComponentLocation=GetMesh()->GetComponentTransform().TransformPosition(WheelState.InitialLocalLocation+WheelState.WheelSetup->TraceStartOffset) ;
	TArray<FHitResult>	 TraceResults;

	const FVector DirectionVector=WheelState.SuspAngle==0.0f?GetMesh()->GetUpVector():GetMesh()->GetUpVector().RotateAngleAxis(WheelState.SuspAngle,FVector(1,0,0));
	UKismetSystemLibrary::SphereTraceMulti(GetWorld(),ComponentLocation,ComponentLocation+(DirectionVector*-1*WheelState.WheelSetup->SuspensionLength),WheelState.WheelSetup->WheelRadius,VehicleState.VehicleData->SuspensionTraceTypeQuery,true,ActorsToIgnore,GModularVehicleDebugParams.ShowSuspensionDebug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,TraceResults,true);
	FHitResult TraceResult;
	if(TraceResults.IsValidIndex(0))
	TraceResult=TraceResults[0];
	const float CurrentLen=FMath::Max<float>(0,WheelState.WheelSetup->SuspensionRest-TraceResult.Time);
	const float DampingCorrection=(((CurrentLen-WheelState.PreviousLen)*VehicleState.VehicleData->DampingCorrectionMultiplier*WheelState.WheelSetup->Stiffness))/DeltaTime;
	if(TraceResult.bBlockingHit&&GetOwnerRole()==ENetRole::ROLE_Authority)
	{
		if(TraceResult.Time<WheelState.WheelSetup->SuspensionRest)
		{
			WheelState.WheelLoad=((FVector(0,0,1)*(WheelState.WheelSetup->Stiffness+DampingCorrection)*(CurrentLen)));
			GetMesh()->AddForceAtLocation(WheelState.WheelLoad,TraceResult.TraceStart);
		}else
		{
			
		}
	}
	
	WheelState.PreviousLen=CurrentLen;
	WheelState.HitResult=TraceResult;

	
	//logic
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(GModularVehicleDebugParams.ShowSuspensionDebug)
	{
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+(DeltaTime*(FVector(0,0,1)*(WheelState.WheelSetup->Stiffness/1000)*(CurrentLen))),FColor::Red,false,-1,0,5);
	

	}


	#endif

}


void UModularMovementComponent::SimulateWheel(FWheelState& WheelState, float DeltaTime,
    USceneComponent* ArcadeWheel)
{

//maybe you'll need to make a simplified version
//If We are Tank we need to apply a steering value because lateral friction calculation is dependant on steering Value


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
	WheelState.DriveTorque=WheelState.DriveTorque*WheelState.TorqueTransferFactor;
	WheelState.BrakeTorque=BrakeInput*WheelState.WheelSetup->BrakeTorque;
	
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
	const FVector  LocalWheelVelocity = WorldTransform.InverseTransformVector(GetMesh()->GetPhysicsLinearVelocityAtPoint(WheelState.HitResult.TraceStart));
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	WheelState.SlipAngle = FMath::Atan2(GroundVelocityVector.Y, GroundVelocityVector.X);
	float FinalLongitudinalForce = 0.f;
	FVector ForceFromFriction=FVector::ZeroVector;
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

			 float	SideSlipModifier = 1.0f;
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

			static float DynamicFrictionLateralScaling =0.75f;
			
			if (Locked || WheelState.Spinning)
			{
				SideSlipModifier *=WheelState.WheelSetup->SideSlipModifier;
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
	
	if(GetOwnerRole()==ENetRole::ROLE_Authority)
	{
		FVector FrictionForceLocal = ForceFromFriction;
		FrictionForceLocal =SteeringRotator.RotateVector(FrictionForceLocal);
		FVector GroundZVector = WheelState.HitResult.ImpactNormal;
		FVector GroundXVector = FVector::CrossProduct(GetMesh()->GetRightVector(), GroundZVector);
		FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
		FMatrix Mat(GroundXVector, GroundYVector, GroundZVector, GetMesh()->GetComponentLocation());
		FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);
		FVector RightVector=GetMesh()->GetRightVector();
		
		GetMesh()->AddForce(UKismetMathLibrary::Dot_VectorVector(RightVector,GetMesh()->GetPhysicsLinearVelocity())*RightVector*VehicleState.VehicleData->SideSlip*-1	);
		GetMesh()->AddForceAtLocation(FrictionForceVector,WheelState.HitResult.TraceStart);
		if(GModularVehicleDebugParams.ShowDrawFriction)
		{
			DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+(FrictionForceVector/500),FColor::Green,false,-1,0,15);
			DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+GetMesh()->GetForwardVector()*AppliedLinearDriveForce,FColor::Red,false);
			
		}
	}
}

void UModularMovementComponent::StopActiveMovement()
{
	Super::StopActiveMovement();
	SetThrottleInput(0.0f);
	
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

bool UModularMovementComponent::CylinderTrace(UPrimitiveComponent* Shape, FVector Start, FVector End,
	TArray<FHitResult>& result,const FComponentQueryParams& Params)
{
return 	GetWorld()->ComponentSweepMulti(result,Shape,Start,End,FRotator::ZeroRotator.Quaternion(),Params);
	//TODO ADDDraw
}


bool UModularMovementComponent::ServerUpdateState_Validate(uint16 InQuantizeInput)
{
	return true;
}

void UModularMovementComponent::ServerUpdateState_Implementation(uint16 InQuantizeInput)
{
	
	const int32 QThrottleInput = static_cast<int8>(InQuantizeInput & 0xFF);
	const int32 QSteeringInput = static_cast<int8>(((InQuantizeInput >> 8) & 0x7F) << 1) / 2;
	const int32 QHandbrakeInput = (InQuantizeInput >> 15) & 1;
//TODO Qhandbrake
	SetThrottleInput(QThrottleInput / 127.f);
	SetSteeringInput(QSteeringInput / 63.f);
	
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
	if(GModularVehicleDebugParams.ShowInputProcessingDebug)
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

	
	if(RawThrottleInput==0.0&&VehicleState.VehicleData->SteerType==Tank)
	{
		NewThrottleInput=RawThrottleInput=FMath::Clamp((FMath::Abs<float>(VehicleState.TrackRight.TorqueTransfer)+FMath::Abs<float>(VehicleState.TrackLeft.TorqueTransfer))/2,0.f,1.f);
	}

	//Debug
if(GModularVehicleDebugParams.ShowInputProcessingDebug)
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
	if(GModularVehicleDebugParams.ShowGearboxLog)
	UE_LOG(LogArcadeVehicle,Warning,TEXT("Change gear called with %d "),GearNum);
	if(VehicleState.VehicleData->Gears.IsValidIndex(GearNum))
	{
		if(bImmediate)
		{
			OnGearChange.Broadcast(VehicleState.CurrentGear,GearNum,true);
			VehicleState.CurrentGear=VehicleState.TargetGear=GearNum;
			
		}else
		{
			VehicleState.TargetGear=GearNum;
			OnGearChange.Broadcast(VehicleState.CurrentGear,GearNum,false);
			VehicleState.CurrentGearChangeTime=VehicleState.VehicleData->GearChangeTime;
		}
	}
}




void UModularMovementComponent::UpdateReplicatedCosmeticData()
{
	RepCosmeticData.EngineRPM = VehicleState.CurrentRpmRatio * 255.f;
	
	RepCosmeticData.CurrentGear=VehicleState.CurrentGear;
}

void UModularMovementComponent::OnRep_RepCosmeticData()
{
	
	VehicleState.CurrentRpmRatio=RepCosmeticData.EngineRPM/255.f;
	VehicleState.CurrentRpm=UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,0,1,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
	if(VehicleState.CurrentGear!=RepCosmeticData.CurrentGear)
	{
		OnGearChange.Broadcast(VehicleState.CurrentGear,RepCosmeticData.CurrentGear,true);
		VehicleState.CurrentGear=RepCosmeticData.CurrentGear;
	
	}

	
}

void UModularMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UModularMovementComponent, bIsSleeping);
	//DOREPLIFETIME(UModularMovementComponent, bIsMovementEnabled);
	DOREPLIFETIME(UModularMovementComponent, RepCosmeticData);
	
	
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