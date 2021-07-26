// @Irajsb 1mohtashamiraj@gmail.com

#include "ModularMovementComponent.h"


#include "ModularWheel.h"
#include "ModularMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "Engine.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsSettings.h"


DECLARE_CYCLE_STAT(TEXT("Modular Tick Component"), STAT_ModularTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Engine"), STAT_ModularEngine, STATGROUP_MovementPhysics);



#define LOCTEXT_NAMESPACE "ModularMovement"

FORCEINLINE float OmegaToRPM(float Omega)
{
	return Omega * 30.f / PI;
}

UModularMovementComponent::UModularMovementComponent()
{

	SetIsReplicated(true);
	
}

void UModularMovementComponent::UpdateComponents()
{
	
	//refresh list of components 
	TArray<UModularWheel*> TempComponents;

	
		for (UActorComponent* Component :GetOwner()->GetComponents())
		{
			if (auto Casted= Cast<UModularWheel>(Component))
			{
				TempComponents.Add(Casted);
			}
		}
	

Components=TempComponents;
}

UMeshComponent* UModularMovementComponent::GetMesh()const
{
	//get mesh of vehicle
	return  Cast<UMeshComponent>(UpdatedComponent);
}


UModularVehicleData* UModularMovementComponent::GetSetup() const
{return  VehicleState.VehicleData;
}

void UModularMovementComponent::SetThrottleInput(float Input)
{
	//Setting RawThrottleInput
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

int UModularMovementComponent::GetNumberOfWheels() const
{
	return Components.Num();
}

int UModularMovementComponent::GetNumberOfDriveWheelsTouchingGround() const
{
	return  VehicleState.DriveWheelsOnGround;
}

FModularGearInfo UModularMovementComponent::GetGearInfo(int Index) const
{
	if(VehicleState.VehicleData->Gears.IsValidIndex(Index))
	{
		return VehicleState.VehicleData->Gears[Index];
	}else
	{
		UE_LOG(LogModularVehicle,Error,TEXT("Wrong GearIndex"));
	}
	const 	FModularGearInfo Gear(1);
	return  Gear;
}

void UModularMovementComponent::InitializeComponent()
{


	Super::InitializeComponent();
	if(!VehicleState.VehicleData)
	{
		UE_LOG(LogModularVehicle,Error,TEXT("Assign The Vehicle DataAsset "));

		return;
	}
	
	UpdateNavAgent(*GetOwner());
	UpdateComponents();
	
	//Finding Idle
	
	for (int Index=0 ;Index!=VehicleState.VehicleData->Gears.Num();++Index)
	{
		if (VehicleState.VehicleData->Gears[Index].GearRatio==0)
		{
			VehicleState.IdleGear=Index;
			RepCosmeticData.TargetGear=	RepCosmeticData.CurrentGear=VehicleState.TargetGear=VehicleState.CurrentGear=VehicleState.IdleGear+1;
		}
	}

	//allow wheels to init  the variables that they need
	for(UModularWheel* Component: Components)
	{
	Component->SetupWheels(this);
	
	}
	

	//Setup Mesh
	
	UMeshComponent* MeshComponent=	GetMesh();
	MeshComponent->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	MeshComponent->BodyInstance.bSimulatePhysics = true;
	MeshComponent->BodyInstance.bNotifyRigidBodyCollision = true;
	MeshComponent->BodyInstance.bUseCCD = true;
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetCanEverAffectNavigation(false);
	if(GetOwnerRole()==ENetRole::ROLE_SimulatedProxy)
	{
		GetMesh()->SetSimulatePhysics(false);
	}
	OnCalculateCustomPhysics.BindUObject(this,&UModularMovementComponent::VehicleTick);

	//Calculate Constants
	AirDragConstant=0.5*VehicleState.VehicleData->AirDragCoefficient*VehicleState.VehicleData->VehicleFrontArea;
	RollingResistanceConstant=30*AirDragConstant;
}

void UModularMovementComponent::VehicleTick(float DeltaTime, FBodyInstance* BodyInstance)
{

	MODULAR_CYCLE_COUNTER(STAT_ModularTickComponent)
	const float fDeltaTime=FMath::Min<float>(DeltaTime,0.0633);
	
	//Check data 
	if(!VehicleState.VehicleData)
	{
		UE_LOG(LogModularVehicle,Error,TEXT("Assign The Vehicle DataAsset "));
	
		return;
	}
	
	CaptureState(fDeltaTime);
	if(ShouldReplicateInput())
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
		

		
		
		float WheelTorque;

		UpdateAirDrag();
		UpdateEngine(fDeltaTime,WheelTorque);
		
		
		UpdateGearBox(fDeltaTime);
		UpdateWheels(fDeltaTime,WheelTorque);
		UpdateReplicatedCosmeticData();
	}
	if(ShouldProcessCosmetics())
	{
		ApplyServerCorrection(DeltaTime);
	
		
	}

	
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	
	
	if(!UPhysicsSettings::Get()->bSubstepping)
	{
		 VehicleTick(DeltaTime,nullptr);
	}else
	{
 		GetMesh()->GetBodyInstance()->AddCustomPhysics(OnCalculateCustomPhysics);
	}
	
	
}

void UModularMovementComponent::CaptureState(float DeltaTime)
{
	VehicleState.DriveWheelsOnGround=0;
	for(UModularWheel* Component: Components)
	{
		if(Component->WheelState.HitResult.bBlockingHit&&Component->WheelState.WheelSetup->ApplyDriveForce)
		{
			VehicleState.DriveWheelsOnGround+=1;
		}
	}
	
	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity(), GetMesh()->GetForwardVector());


	if(ShouldProcessPhysics())
	{
		if((VehicleState.VehicleData->NetworkMode==EVehicleNetworkMode::ClientAuthoritative&&GetOwnerRole()==ENetRole::ROLE_AutonomousProxy)||(VehicleState.VehicleData->NetworkMode!=EVehicleNetworkMode::ClientAuthoritative&&GetOwnerRole()==ENetRole::ROLE_Authority))
		{
			//TODO : Capture Movement
		}
	}
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
				 if (RawThrottleInput > KINDA_SMALL_NUMBER &&VehicleState.CurrentGear <= VehicleState.IdleGear && VehicleState.TargetGear <= VehicleState.IdleGear)
				{
				SetTargetGear(VehicleState.IdleGear+1, true);
				}
			}else
			{// situations when  car is moving fast and needs to change gear 

				//if car is moving in forward speed and  its in back gear (happens after a -180 degree flip in reverse
				if(VehicleState.ForwardSpeed>KINDA_SMALL_NUMBER&&RawThrottleInput>KINDA_SMALL_NUMBER&&VehicleState.CurrentGear<VehicleState.IdleGear)
				{
					SetTargetGear(VehicleState.IdleGear+1, true);
				}
			}
			
	}



	// not currently changing gear, also don't want to change up because the wheels are spinning up due to having no load
	if(VehicleState.CurrentGear>VehicleState.IdleGear)
	{
		if (VehicleState.CurrentGearChangeTime==0.f && AllowedToChangeGear())
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
}
	if (VehicleState.CurrentGear != VehicleState.TargetGear)
	{
		VehicleState.CurrentGearChangeTime -= DeltaTime;
		if (VehicleState.CurrentGearChangeTime <= 0.f)
		{
			VehicleState.CurrentGearChangeTime = 0.f;
			OnGearChange.Broadcast(VehicleState.CurrentGear,VehicleState.TargetGear,true);
			VehicleState.CurrentGear =VehicleState.TargetGear;


			
		}
	}
}

void UModularMovementComponent::UpdateEngine(float DeltaTime,float& WheelTorque )
{

	MODULAR_CYCLE_COUNTER(STAT_ModularEngine)
	
	float HighestOmega=0;
	VehicleState.DriveWheelsOnGround=0;
	//Get fastest wheel that is attached to engine 
	for(UModularWheel* Component: Components)
	{
		
		const float ComponentOmega=	FMath::Abs(Component ->GetFastestWheelOmegaSpeed());
		if(Component->GetWheelState()->HitResult.bBlockingHit&&Component->GetWheelState()->WheelSetup->ApplyDriveForce)
		{
			VehicleState.DriveWheelsOnGround++;
		}
		if(ComponentOmega>HighestOmega)
		{
			HighestOmega=ComponentOmega;
		}
	}

	 
	const float ThrottleInput=CalcThrottleInput();
	const float WheelRPM =OmegaToRPM(HighestOmega);
	//Calculate RPM 
	VehicleState.CurrentRpm=FMath::Clamp<float>(WheelRPM *GetGearInfo(VehicleState.CurrentGear).GearRatio*VehicleState.VehicleData->DifferentialRatio,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
	VehicleState.CurrentRpmRatio= UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm,0,1);
	//TODO Refactor
	if(VehicleState.VehicleData->ZeroRpmWhenShifting&&VehicleState.CurrentGearChangeTime>0)
	{
		VehicleState.CurrentRpm=0;
		VehicleState.CurrentRpmRatio=0;
	}else
	{

		if(VehicleState.DriveWheelsOnGround==0)
		{
			VehicleState.CurrentRpm=FMath::Clamp<float>((VehicleState.VehicleData->MaxRpm*ThrottleInput),VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
		}
		
		//Engine Torque 
		
		
		//Use curve 
			const float EngineTorque=ThrottleInput* VehicleState.VehicleData->EngineTorqueCurve.GetRichCurve()->Eval(VehicleState.CurrentRpm);
		
		//Gearbox 
		const float TransmissionTorque=GetGearInfo(VehicleState.CurrentGear).GearRatio* VehicleState.VehicleData->TransmissionEfficiency*VehicleState.VehicleData->DifferentialRatio;
		//Final 

		
		WheelTorque=EngineTorque*TransmissionTorque;

	}
	


	
}

void UModularMovementComponent::UpdateAirDrag() const
{
const FVector BodyVelocity=	GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity()/100.f;//CM/s To Meter/s
const FVector DragForce= BodyVelocity*BodyVelocity.Size()*AirDragConstant*-1;
GetMesh()->GetBodyInstance()->AddForceAtPosition(SIForceToUnrealForce(DragForce),GetMesh()->GetCenterOfMass());
}

void UModularMovementComponent::UpdateWheels(float DeltaTime, float WheelTorque)
{
	//Capturing inputs
	//Steer
	SteeringInput=CalcSteeringInput(DeltaTime);
	const float SteerSpeedScale =	VehicleState.VehicleData->SteerCurve.GetRichCurve()->IsEmpty()?1:VehicleState.VehicleData->SteerCurve.GetRichCurve()->Eval(VehicleState.ForwardSpeed*0.036/*CmSToKmH*/) ;
	const 	float UseSteeringValue = SteeringInput * SteerSpeedScale;
	//Brake 
	BrakeInput=CalcBrakeInput();

	
	for(UModularWheel* Component: Components)
	{

		
		//calc and Apply susp forces 
		Component->UpdateSuspension(DeltaTime,this);
		
		//Apply Engine Torque 
		if(VehicleState.VehicleData->ScaleDriveTorqueToNumberOfWheels)
		{
			Component->SetDriveTorqueOnWheels(VehicleState.DriveWheelsOnGround!=0 ?WheelTorque/VehicleState.DriveWheelsOnGround:0);
		}
		else
		{
			Component->SetDriveTorqueOnWheels(WheelTorque);
		}


		Component->WheelState.BrakeTorque=BrakeInput*Component->WheelState.WheelSetup->BrakeTorque;
		if(HandBrakeInput)
		{
			if(Component->WheelState.WheelSetup->HandBrakeTorque!=0.0f)
			{
				Component->WheelState.BrakeTorque=Component->WheelState.WheelSetup->HandBrakeTorque;
				Component->WheelState.DriveTorque=0;
				
			}
		}
		
		//Apply Steering
		Component->UpdateSteering(DeltaTime,this,UseSteeringValue);
		
		//Apply those torques 
		Component->UpdateForces(DeltaTime,this);


		//TODO Optionial this 
		Component->UpdateAnimation(DeltaTime,this);
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
		bool AIDebug=false;
		UKismetSystemLibrary::LineTraceSingle(World,VehicleLocation,VehicleLocation+VehicleDirection*TraceLenForward,VehicleState.VehicleData->SuspensionTraceTypeQuery,false,ActorsToIgnore,AIDebug?EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,HitResultF,true);
		UKismetSystemLibrary::LineTraceSingle(World,VehicleLocation,VehicleLocation+-1*VehicleDirection*TraceLenBackWard,VehicleState.VehicleData->SuspensionTraceTypeQuery,false,ActorsToIgnore,AIDebug?EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,HitResultB,true);

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
			OutState=Neutral;
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
			OutState=Neutral;
		}
		//if back is blocked forward is open we stop reversing
		if(HitResultB.bBlockingHit&&!HitResultF.bBlockingHit)
		{
			VehicleState.LockCurrentStateDelta=0.5;
			OutState=Neutral;
		}
	}
	

	return OutState;
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
	case EAIVehicleState::Neutral:
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
	


	VehicleState.AIPreviousThrottle=RawThrottleInput;
}






void UModularMovementComponent::StopActiveMovement()
{
	Super::StopActiveMovement();
	SetThrottleInput(0.0f);
	
}

void UModularMovementComponent::ApplyServerCorrection(float DeltaTime)
{

	
}

float UModularMovementComponent::CmToM(float In)
{
	return  In*100;
}



bool UModularMovementComponent::ShouldProcessPhysics()const
{
	
if(GetNetMode()==ENetMode::NM_Standalone)
	return  true;
	
	switch (VehicleState.VehicleData->NetworkMode)
	{
case ClientAuthoritative:
		return 	GetOwnerRole()==ENetRole::ROLE_AutonomousProxy;
case ServerAuthoritative:
	return  GetOwnerRole()==ENetRole::ROLE_Authority;
case ClientPredictive:
	return  GetOwnerRole()==ENetRole::ROLE_Authority|| GetOwnerRole()==ENetRole::ROLE_AutonomousProxy;
default:
	return false;
	}
}

bool UModularMovementComponent::ShouldProcessCosmetics()const
{
return 	GetNetMode()==ENetMode::NM_Standalone||(GetNetMode()==ENetMode::NM_Client&&GetOwnerRole()!=ENetRole::ROLE_Authority);
}

bool UModularMovementComponent::ShouldReplicateInput()const
{
	return 	(GetPawnOwner()->GetLocalRole()!=ENetRole::ROLE_Authority&&GetPawnOwner()->IsLocallyControlled());
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
	
	HandBrakeInput = QHandbrakeInput==1?true:false;

	//LastUserSteeringInput = QSteeringInput;
}


float UModularMovementComponent::CalcSteeringInput(float DeltaTime)
{
	if(SteeringInput==RawSteeringInput)
	{
		return  SteeringInput;
	}
	float SteerDir=	FMath::Sign(RawSteeringInput);
	//Find steering direction
	if(SteerDir==0)
	{
	SteerDir=FMath::Sign(SteeringInput)*-1;
	}	
	
	if(SteeringInput<FMath::Abs(RawSteeringInput))
	{
		//input rise
		SteeringInput = SteeringInput +SteerDir  *VehicleState.VehicleData->SteerInputRise * DeltaTime;
		SteeringInput=FMath::Clamp(SteeringInput,-1.0f,1.0f);
	}else
	{
		//input fall
		SteeringInput = SteeringInput + SteerDir *VehicleState.VehicleData->SteerInputFall * DeltaTime;
		
		SteeringInput=FMath::Clamp(SteeringInput,0.0f,RawSteeringInput);
		//if target is zero
		if(RawSteeringInput==0)
		{
			if(SteerDir>0)
			{
				SteeringInput=FMath::Min(0.0f,SteeringInput);
			}else
			{
				SteeringInput=FMath::Max(0.0f,RawSteeringInput);
			}
		}
	}
	
	

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
		if (RawBrakeInput > 0.f &&VehicleState.CurrentGear < VehicleState.IdleGear /*PVehicle->GetTransmission().GetTargetGear() < 0/*ForwardSpeed < -WrongDirectionThreshold*/)
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
			//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if ((RawThrottleInput > 0.f && VehicleState.TargetGear < VehicleState.IdleGear) || (RawThrottleInput < 0.f && VehicleState.TargetGear > VehicleState.IdleGear))
			{
				NewThrottleInput = 0.f;
			}
	}

	
	if(RawThrottleInput==0.0&&VehicleState.VehicleData->SteerType==Tank)
	{
		NewThrottleInput=RawThrottleInput=FMath::Clamp((FMath::Abs<float>(VehicleState.TrackRight.TorqueTransfer)+FMath::Abs<float>(VehicleState.TrackLeft.TorqueTransfer))/2,0.f,1.f);
	}


	
	return NewThrottleInput;
}

void UModularMovementComponent::SetTargetGear(int32 GearNum, bool bImmediate)
{
	
	if(VehicleState.VehicleData->Gears.IsValidIndex(GearNum))
	{
		if(bImmediate||VehicleState.VehicleData->GearChangeTime==0.f)
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

bool UModularMovementComponent::AllowedToChangeGear()
{
	
	for(UModularWheel* Component: Components)
	{
		
		if(Component->GetWheelState()->WheelStatus==Spinning||Component->GetWheelState()->WheelStatus==Locked)
		{
			return  false;
		}
	
	}
	return true;
}


void UModularMovementComponent::UpdateReplicatedCosmeticData()
{
	RepCosmeticData.EngineRPM = VehicleState.CurrentRpmRatio * 255.f;
	
	RepCosmeticData.CurrentGear=VehicleState.CurrentGear;
	RepCosmeticData.TargetGear=VehicleState.TargetGear;
}

void UModularMovementComponent::OnRep_RepCosmeticData()
{
	
	VehicleState.CurrentRpmRatio=RepCosmeticData.EngineRPM/255.f;
	VehicleState.CurrentRpm=UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,0,1,VehicleState.VehicleData->IdleRpm,VehicleState.VehicleData->MaxRpm);
	
	if(VehicleState.CurrentGear!=RepCosmeticData.CurrentGear)
	{
		OnGearChange.Broadcast(VehicleState.CurrentGear,RepCosmeticData.CurrentGear,true);
		VehicleState.CurrentGear=RepCosmeticData.CurrentGear;
	
	}if(VehicleState.TargetGear!=RepCosmeticData.TargetGear)
	{
		//
		VehicleState.TargetGear=RepCosmeticData.TargetGear;
	}

	
}



void UModularMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	
	DOREPLIFETIME(UModularMovementComponent, RepCosmeticData);
	DOREPLIFETIME(UModularMovementComponent,RepMovement);
	
}
#undef LOCTEXT_NAMESPACE