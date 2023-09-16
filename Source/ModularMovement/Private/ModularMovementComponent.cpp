// Aurelion 2023 Iraj Mohtasham . For distribution in epic games marketplace

#include "ModularMovementComponent.h"


#include "ModularWheel.h"
#include "ModularMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "Engine.h"
#include "Utility/ModuarVehicleDebugger.h"
#include "ModularAsyncCallBack.h"
#include "ModularGearBox.h"

#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"



DECLARE_CYCLE_STAT(TEXT("Modular Tick Component"), STAT_ModularTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Engine"), STAT_ModularEngine, STATGROUP_MovementPhysics);


//TODO : Throttle input rise
#define LOCTEXT_NAMESPACE "ModularMovement"

FORCEINLINE float OmegaToRPM(float Omega)
{
	return Omega * 30.f / PI;
}

UModularMovementComponent::UModularMovementComponent()
{
	SetIsReplicatedByDefault(true);
	
	
}

void UModularMovementComponent::UpdateComponents(const TArray<UModularWheel*> AdditionalWheels)
{
	//refresh list of components 
	TArray<UModularWheel*> TempComponents;


	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (auto Casted = Cast<UModularWheel>(Component))
		{
			TempComponents.Add(Casted);
		}
	}
	//Gather wheels from any possible child actors to support flexibility 
	TArray<AActor* >Children;
	GetOwner()->GetAllChildActors(Children);
	for (const auto Actor:Children)
	{

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (auto Casted = Cast<UModularWheel>(Component))
			{
				TempComponents.Add(Casted);
			}
		}
	}

	for(const auto Wheel : AdditionalWheels)
	{
		if(IsValid(Wheel))
		{
			if(!IsValid(Wheel->WheelState.WheelSetup))
			{
				Wheel->SetupWheels(this);
			}
		TempComponents.Add(Wheel);

			
		}
	}
	Components = TempComponents;
}


TArray<UModularWheel*> UModularMovementComponent::GetWheels()
{
	return Components;
}

UMeshComponent* UModularMovementComponent::GetMesh() const
{
	//get mesh of vehicle
	return Cast<UMeshComponent>(UpdatedComponent);
}


UBaseVehicleData* UModularMovementComponent::GetSetup() const
{
	return VehicleState.VehicleData;
}

void UModularMovementComponent::SetThrottleInput(float Input)
{
	//Setting RawThrottleInput
	RawThrottleInput = FMath::Clamp<float>(Input, -1.f, 1.f);
}

void UModularMovementComponent::SetSteeringInput(float Input)
{
	RawSteeringInput = FMath::Clamp<float>(Input, -1.f, 1.f);
}

void UModularMovementComponent::SetBrakeInput(float Brake)
{
	RawBrakeInput = FMath::Clamp(Brake, -1.0f, 1.0f);
}

void UModularMovementComponent::SetHandBrakeInput(bool Brake)
{
	HandBrakeInput = Brake;
}

int UModularMovementComponent::GetNumberOfWheels() const
{
	return Components.Num();
}

int UModularMovementComponent::GetNumberOfDriveWheelsTouchingGround() const
{
	return VehicleState.DriveWheelsOnGround;
}

float UModularMovementComponent::GetRPMRatio()
{
	return UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,
																	   GetSetup()->GetIdleRPM(),
																	   GetSetup()->GetMaxRPM(), 0, 1);
}



void UModularMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if(VehicleState.VehicleDataClass.LoadSynchronous())
	{
		VehicleState.VehicleData=NewObject<UBaseVehicleData>(this,VehicleState.VehicleDataClass.Get());
	}
	
	if (!GetSetup())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("Assign The Vehicle DataAsset "));

		return;
	}

	

	UpdateNavAgent(*GetOwner());
	UpdateComponents({});



	//allow wheels to init  the variables that they need
	for (UModularWheel* Component : Components)
	{
		Component->SetupWheels(this);
	}


	//Setup Mesh

	UMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	MeshComponent->BodyInstance.bSimulatePhysics = true;
	MeshComponent->BodyInstance.bNotifyRigidBodyCollision = true;
	MeshComponent->BodyInstance.bUseCCD = true;
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetCanEverAffectNavigation(false);
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		GetMesh()->SetSimulatePhysics(false);
	}

	
	

	//Calculate Constants
	AirDragConstant = GetSetup()->GetAirDragConstant();
	//0.5*GetSetup()->AirDragCoefficient*GetSetup()->VehicleFrontArea;
	RollingResistanceConstant = 30 * AirDragConstant;


	//Try Find Debugger
	ModularVehicleDebugger = Cast<UModularVehicleDebugger>(
		GetOwner()->GetComponentByClass(UModularVehicleDebugger::StaticClass()));
}

void UModularMovementComponent::VehicleTick(float DeltaTime, FBodyInstance* BodyInstance)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularTickComponent)
	const float fDeltaTime = FMath::Min<float>(DeltaTime, 0.0633);

	//Check data 
	if (!GetSetup())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("Assign The Vehicle DataAsset "));

		return;
	}

	

	if (ShouldProcessPhysics())
	{
		SteeringInput = CalcSteeringInput(DeltaTime);
		ThrottleInput = CalcThrottleInput(DeltaTime);
		BrakeInput = CalcBrakeInput();

		float WheelTorque=0.f;

		UpdateAirDrag();
		UpdateEngine(fDeltaTime, WheelTorque);


		
		UpdateWheels(fDeltaTime, WheelTorque);
		UpdateReplicatedCosmeticData();
	}
	if (ShouldProcessCosmetics())
	{
		//
	}
}

void UModularMovementComponent::PreTick(FPhysScene_Chaos* Scene, float DeltaTime)
{
	if(AsyncCallBack == nullptr)
		return;

	FAsyncPhysicsInput* AsyncInput = AsyncCallBack->GetProducerInputData_External();

	if(AsyncInput == nullptr)
		return;

	UWorld* World = Scene->GetOwningWorld();

	if(World == nullptr)
		return;

	AsyncInput->Reset();

	AsyncInput->World = World;
	
	
	
		AsyncInput->Components.Add(this);
	
}

void UModularMovementComponent::PhysicsCallBack(float DeltaTime)
{
	VehicleTick(DeltaTime,nullptr);
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	
	if (ShouldReplicateInput())
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
		GetSetup()->GetGearBox()->Update(DeltaTime,this);
		CaptureState(DeltaTime);
	}

	for (UModularWheel* Component : Components)
	{

		Component->UpdateAnimation(DeltaTime,this);
	}

}

void UModularMovementComponent::BeginPlay()
{


	GetWorld()->GetPhysicsScene()->OnPhysSceneStep.AddUObject(this,&UModularMovementComponent::PreTick);

	
	
	if(!AsyncCallBack)
	{
		AsyncCallBack = GetWorld()->GetPhysicsScene()->GetSolver()->CreateAndRegisterSimCallbackObject_External<FModularAsyncCallBack>();
	}
	Super::BeginPlay();
	GetSetup()->GetGearBox()->SetupGearBox();
}

void UModularMovementComponent::CaptureState(float DeltaTime)
{
	VehicleState.DriveWheelsOnGround = 0;
	for (const UModularWheel* Component : Components)
	{
		if (Component->WheelState.HitResult.bBlockingHit && Component->WheelState.ApplyDriveForce)
		{
			VehicleState.DriveWheelsOnGround += 1;
		}
	}

	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity(),
	                                                GetMesh()->GetForwardVector());

	VehicleState.SideSpeed = FVector::DotProduct(GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity(),
													GetMesh()->GetRightVector());

	if (ShouldProcessPhysics())
	{
		if ((GetSetup()->GetNetworkMode() == ClientAuthoritative && GetOwnerRole() == ROLE_AutonomousProxy) || (
			GetSetup()->GetNetworkMode() !=
			ClientAuthoritative && GetOwnerRole() == ROLE_Authority))
		{

		}
	}
}



void UModularMovementComponent::UpdateEngine(float DeltaTime, float& WheelTorque)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularEngine)

	
	
	float AxleRPM = 0;
	VehicleState.DriveWheelsOnGround = 0;
	//Get fastest wheel that is attached to engine 
	for (UModularWheel* Component : Components)
	{
		const float ComponentOmega = FMath::Abs(Component->GetFastestWheelOmegaSpeed());
		if (Component->GetWheelState()->HitResult.bBlockingHit && Component->WheelState.ApplyDriveForce)
		{
			VehicleState.DriveWheelsOnGround++;
		}
		if (ComponentOmega > AxleRPM)
		{
			AxleRPM = ComponentOmega;
		}
	}


	const float TargetRPM=FMath::Clamp<float>(
                          		AxleRPM * GetSetup()->GetGearBox()->GetDriveRatio(),
                          		GetSetup()->GetIdleRPM(), GetSetup()->GetMaxRPM());
                          		
	VehicleState.EngineRads =  FMath::FInterpConstantTo(VehicleState.EngineRads,TargetRPM,DeltaTime,0.1*VehicleState.VehicleData->GetMaxRPM());
	VehicleState.EngineRads=FMath::Min(VehicleState.EngineRads,VehicleState.VehicleData->GetMaxRPM());

	VehicleState.CurrentRpm=OmegaToRPM(VehicleState.EngineRads);
	


	

	
	


	
	//TODO Refactor
	if (GetSetup()->ShouldZeroRpmWhenShifting() && GetSetup()->GetGearBox()->IsChangingGear() )
	{
		VehicleState.CurrentRpm = 0;
	
	}
	else
	{
		if (VehicleState.DriveWheelsOnGround == 0)
		{
			VehicleState.CurrentRpm = FMath::Clamp<float>((GetSetup()->GetMaxRPM() * ThrottleInput),
			                                              GetSetup()->GetIdleRPM(), GetSetup()->GetMaxRPM());
		}

		//Engine Torque 


		//Use curve 
		const float EngineTorque = ThrottleInput * GetSetup()->GetTorqueForRPM(VehicleState.CurrentRpm);

		//Gearbox 
		const float TransmissionTorque = GetSetup()->GetGearBox()->GetDriveRatio();
		//Final 


		WheelTorque = EngineTorque * TransmissionTorque;

		if (ModularVehicleDebugger)
		{
			//Set Torques and throttle
			ModularVehicleDebugger->EngineTorque=EngineTorque;
			ModularVehicleDebugger->WheelTorque=WheelTorque;
			ModularVehicleDebugger->ThrottleInput=ThrottleInput;
			
		}
	}
}

void UModularMovementComponent::UpdateAirDrag() const
{
	const FVector BodyVelocity = GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity() / 100.f; //CM/s To Meter/s
	const FVector DragForce = BodyVelocity * BodyVelocity.Size() * AirDragConstant * -1;
	GetMesh()->GetBodyInstance()->AddForceAtPosition(
		SIForceToUnrealForce(DragForce), GetMesh()->GetCenterOfMass(), true);
}

void UModularMovementComponent::UpdateWheels(float DeltaTime, float WheelTorque)
{
	//Capturing inputs
	//Steer

	

	const float UseSteeringValue = SteeringInput ;


	if (VehicleState.VehicleData->GetSteerType() == Tank)
	{
		const float LeftTrackInput = UseSteeringValue;
		const float RightTrackInput = -UseSteeringValue;
		VehicleState.TrackLeft.TorqueTransfer = 0;
		VehicleState.TrackRight.TorqueTransfer = 0;
		if (FMath::Abs(RawThrottleInput) > SMALL_NUMBER)
		{
			VehicleState.TrackLeft.TorqueTransfer =
				RawThrottleInput + LeftTrackInput;
			VehicleState.TrackRight.TorqueTransfer =
				RawThrottleInput + RightTrackInput;
		}
		else
		{
			VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(
				RawThrottleInput) + LeftTrackInput;
			VehicleState.TrackRight.TorqueTransfer = FMath::Abs(
				RawThrottleInput) + RightTrackInput;
		}
	}

	for (UModularWheel* Component : Components)
	{
		//calc and Apply susp forces 
		Component->UpdateSuspension(DeltaTime, this);

		//Apply Engine Torque 
		if (GetSetup()->ShouldScaleDriveTorqueToNumberOfWheels())
		{
			Component->SetDriveTorqueOnWheels(VehicleState.DriveWheelsOnGround != 0
				                                  ? WheelTorque / VehicleState.DriveWheelsOnGround
				                                  : 0);
		}
		else
		{
			Component->SetDriveTorqueOnWheels(WheelTorque);
		}


		Component->WheelState.BrakeTorque = BrakeInput * Component->WheelState.WheelSetup->BrakeTorque;
		Component->WheelState.IsHandBrakeTorque = false;
		if (HandBrakeInput)
		{
			if (Component->WheelState.AffectedByHandBrake)
			{
				Component->WheelState.BrakeTorque = Component->WheelState.WheelSetup->HandBrakeTorque;
				Component->WheelState.IsHandBrakeTorque = true;
				Component->WheelState.DriveTorque = 0;
			}
		}


		//Apply Steering
		Component->UpdateSteering(DeltaTime, this, UseSteeringValue);

		//Apply those torques 
		Component->UpdateForces(DeltaTime, this);


		
	}
}


EAIVehicleState UModularMovementComponent::DetermineAIState(float ForwardFactor, float DeltaTime)
{
	UWorld* World = GetWorld();
	const FVector VehicleLocation = GetOwner()->GetActorLocation()+FVector(0,0,GetOwner()->GetRootComponent()->GetLocalBounds().BoxExtent.Z);
	FHitResult HitResultF;
	FHitResult HitResultB;
	const FVector VehicleDirection = GetMesh()->GetForwardVector();

	//trace
	if (World)
	{
		
		ActorsToIgnore.Add(GetOwner());


		const float TraceLenForward = GetSetup()->GetAITraceLength() +FMath::Max(0, VehicleState.ForwardSpeed)*VehicleState.VehicleData->GetAITraceSpeedMultiplier();
		const float TraceLenBackWard = GetSetup()->GetAITraceLength() +FMath::Min(0, VehicleState.ForwardSpeed)*VehicleState.VehicleData->GetAITraceSpeedMultiplier();
		DrawDebugString(GetWorld(),GetMesh()->GetComponentLocation(),FString::SanitizeFloat(TraceLenForward),nullptr,FColor::Red,0);
		//start traces
		bool AIDebug = true;
		UKismetSystemLibrary::LineTraceSingle(World, VehicleLocation,
		                                      VehicleLocation + VehicleDirection * TraceLenForward,
		                                      GetSetup()->GetSuspensionTraceTypeQuery(), false, ActorsToIgnore,
		                                      AIDebug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		                                      HitResultF, true);
		UKismetSystemLibrary::LineTraceSingle(World, VehicleLocation,
		                                      VehicleLocation + -1 * VehicleDirection * TraceLenBackWard,
		                                      GetSetup()->GetSuspensionTraceTypeQuery(), false, ActorsToIgnore,
		                                      AIDebug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		                                      HitResultB, true);
	}
	//determine state
	//turn around state

	VehicleState.LockCurrentStateDelta -= DeltaTime;

	EAIVehicleState OutState = VehicleState.AIState;
	//if state is locked but there is crash risk avoid it
	if (VehicleState.LockCurrentStateDelta > 0)
	{
		if (VehicleState.AIState == TurningAround && HitResultB.bBlockingHit)
		{
			OutState = Neutral;
		}
		if (VehicleState.AIState != TurningAround && HitResultF.bBlockingHit)
		{
			OutState = TurningAround;
		}
		return OutState;
	}
	//state not locked
	//Normally moving forward

	if (VehicleState.AIState == EAIVehicleState::Neutral)
	{
		//if We need to turn or turn around
		if (ForwardFactor < GetSetup()->GetReverseThreshold())
		{
			OutState = TurningAround;
		}
		//forward is  blocked
		if (!HitResultB.bBlockingHit && HitResultF.bBlockingHit)
		{
			OutState = TurningAround;
			VehicleState.LockCurrentStateDelta = 0.5;
		}
	}

	//Turning Around
	else if (VehicleState.AIState == TurningAround)
	{
		if (FMath::IsNearlyEqual(ForwardFactor, 1.0f, 0.1f))
		{
			OutState = Neutral;
		}
		//if back is blocked forward is open we stop reversing
		if (HitResultB.bBlockingHit && !HitResultF.bBlockingHit)
		{
			VehicleState.LockCurrentStateDelta = 0.5;
			OutState = Neutral;
		}
	}


	return OutState;
}


void UModularMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	Super::RequestDirectMove(MoveVelocity, bForceMaxSpeed);

	VehicleState.IsAIVehicle = true;
	VehicleState.DesiredSpeed = 0.0f;
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FVector VehicleLocation = GetOwner()->GetActorLocation();
	const FVector Destination = VehicleLocation + MoveVelocity * DeltaSeconds;
	const FVector Distance = Destination - VehicleLocation;
	const FVector VehicleDirection = GetMesh()->GetForwardVector();
	const float ForwardFactor = FVector::DotProduct(VehicleDirection, Distance.GetSafeNormal());


	float CurrentYaw = Distance.Rotation().Yaw - GetMesh()->GetForwardVector().Rotation().Yaw;
	if (CurrentYaw < -180)
	{
		CurrentYaw += 360;
	}
	else if (CurrentYaw > 180)
	{
		CurrentYaw -= 360;
	}

	const float SteeringPosition = (1 - ((-CurrentYaw + 180) / 180)) * 10;

	VehicleState.AIState = DetermineAIState(ForwardFactor, DeltaSeconds);


	//react to state

	switch (VehicleState.AIState)
	{
	case Neutral:
		SetSteeringInput(SteeringPosition);
		VehicleState.DesiredSpeed = UKismetMathLibrary::MapRangeClamped(
			Distance.Size(), 0, GetSetup()->GetNearGoalDistance(), GetSetup()->GetDesireSpeedNearGoal(),
			GetSetup()->GetDesireSpeedNormal());
		if (ForwardFactor < GetSetup()->GetTurnThreshold())
		{
			VehicleState.DesiredSpeed = GetSetup()->GetDesireSpeedTurning();
		}
		break;
	case TurningAround:
		VehicleState.DesiredSpeed = GetSetup()->GetDesireSpeedTurningAround();
		SetSteeringInput(SteeringPosition > 0 ? -1 : 1);

		break;
	}


	//match desired speed
	const float SpeedDifference = VehicleState.DesiredSpeed - VehicleState.ForwardSpeed * 0.036; //cms to kmh


	if (SpeedDifference > 0)
	{
		//we should add throttle
		SetThrottleInput(FMath::Clamp<float>(SpeedDifference / GetSetup()->GetFullThrottleSpeed(), -1, 1));
	}
	else
	{
		//brake or release throttle 
		SetThrottleInput(FMath::Clamp<float>(SpeedDifference / GetSetup()->GetFullThrottleSpeed(), -1, 1));
	}


	VehicleState.AIPreviousThrottle = RawThrottleInput;
}


void UModularMovementComponent::StopActiveMovement()
{
	Super::StopActiveMovement();
	SetThrottleInput(0.0f);
}



float UModularMovementComponent::CmToM(float In)
{
	return In * 100;
}


bool UModularMovementComponent::ShouldProcessPhysics() const
{
	if (GetNetMode() == NM_Standalone)
	{
		return true;
	}

	switch (GetSetup()->GetNetworkMode())
	{
	case ClientAuthoritative:
		return GetOwnerRole() == ROLE_AutonomousProxy;
	case ServerAuthoritative:
		return GetOwnerRole() == ROLE_Authority;
	case ClientPredictive:
		return GetOwnerRole() == ROLE_Authority || GetOwnerRole() == ROLE_AutonomousProxy;
	default:
		return false;
	}
}

bool UModularMovementComponent::ShouldProcessCosmetics() const
{
	return GetNetMode() == NM_Standalone || (GetNetMode() == NM_Client && GetOwnerRole() != ROLE_Authority);
}

bool UModularMovementComponent::ShouldReplicateInput() const
{
	return (GetPawnOwner()->GetLocalRole() != ROLE_Authority && GetPawnOwner()->IsLocallyControlled());
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

	SetThrottleInput(QThrottleInput / 127.f);
	SetSteeringInput(QSteeringInput / 63.f);

	HandBrakeInput = QHandbrakeInput == 1 ? true : false;

	//LastUserSteeringInput = QSteeringInput;
}


float UModularMovementComponent::CalcSteeringInput(float DeltaTime)
{

	
	
	// Determine the rate to use for interpolation
	const float InterpolationSpeed = (RawSteeringInput!=0.f||FMath::Sign(RawSteeringInput*SteeringInput)==1 ? GetSetup()->GetSteerInputRise() : GetSetup()->GetSteerInputFall());
    
	// Interpolate between the current steering input and the target
	SteeringInput = FMath::FInterpTo(SteeringInput, RawSteeringInput, DeltaTime, InterpolationSpeed);
    
	// Clamp the steering input to ensure it's within valid range
	SteeringInput = FMath::Clamp(SteeringInput, -1.0f, 1.0f);
	
	return SteeringInput;
}

float UModularMovementComponent::CalcBrakeInput() const
{
	if (GetSetup()->ShouldReverseAsBrake())
	{
		float NewBrakeInput = 0.0f;

		// if player wants to move forwards...
		if (RawThrottleInput > 0.f)
		{
			// if vehicle is moving backwards, then press brake
			if (VehicleState.ForwardSpeed < -GetSetup()->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
			}
		}

		// if player wants to move backwards...
		else if (RawThrottleInput < 0.f)
		{
			// if vehicle is moving forwards, then press brake
			if (VehicleState.ForwardSpeed > GetSetup()->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
			}
		}
		// if player isn't pressing forward or backwards...
		else
		{
			if (VehicleState.ForwardSpeed < GetSetup()->GetStopThreshold() && VehicleState.ForwardSpeed > -GetSetup()->
				GetStopThreshold()) //auto brake 
			{
				NewBrakeInput = 1.f;
			}
			else
			{
				NewBrakeInput = GetSetup()->GetIdleBrakeInput();
			}
		}

		return FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
	}
	float NewBrakeInput = FMath::Abs(RawBrakeInput);

	// if player isn't pressing forward or backwards...
	if (RawBrakeInput < SMALL_NUMBER && RawThrottleInput < SMALL_NUMBER)
	{
		if (VehicleState.ForwardSpeed < GetSetup()->GetStopThreshold() && VehicleState.ForwardSpeed > -GetSetup()->
			GetStopThreshold()) //auto brake 
		{
			NewBrakeInput = 1.f;
		}
	}


	return NewBrakeInput;
}

float UModularMovementComponent::CalcThrottleInput(float DeltaTime) const
{
	float NewThrottleInput = RawThrottleInput;

	if (GetSetup()->ShouldReverseAsBrake())
	{
		if (RawBrakeInput > 0.f && GetSetup()->GetGearBox()->IsInReverse())
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
		//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if (RawThrottleInput > 0.f && GetSetup()->GetGearBox()->IsInReverse() || RawThrottleInput < 0.f
				&& !GetSetup()->GetGearBox()->IsInReverse())
			{
				NewThrottleInput = 0.f;
			}
	}

	//Throttle and steer are not discrete in a  tank so we calculate both here
	if (GetSetup()->GetSteerType() == Tank)
	{
		NewThrottleInput = FMath::Clamp(FMath::Abs(RawThrottleInput) + FMath::Abs(SteeringInput), 0.f, 1.f);
	}


	return NewThrottleInput;
}






void UModularMovementComponent::UpdateReplicatedCosmeticData()
{
	RepCosmeticData.EngineRPM = GetRPMRatio() * 255.f;

	RepCosmeticData.CurrentGear = GetSetup()->GetGearBox()->CurrentGear;
	
}

void UModularMovementComponent::OnRep_RepCosmeticData()
{
	
	VehicleState.CurrentRpm = UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm, 0, 1,
	                                                              GetSetup()->GetIdleRPM(), GetSetup()->GetMaxRPM());

	if (const auto CurrentGear = GetSetup()->GetGearBox()->CurrentGear!= RepCosmeticData.CurrentGear)
	{
		OnGearChange.Broadcast(CurrentGear, RepCosmeticData.CurrentGear, true);
		GetSetup()->GetGearBox()->SetCurrentGear(RepCosmeticData.CurrentGear);
	}
	
}


void UModularMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
}
#undef LOCTEXT_NAMESPACE
