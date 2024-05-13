// Aurelion 2023 Iraj Mohtasham . For distribution in epic games marketplace

#include "ModularMovementComponent.h"


#include "ModularWheel.h"
#include "ModularMovement.h"
#include "Misc/MessageDialog.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utility/ModuarVehicleDebugger.h"
#include "ModularAsyncCallBack.h"
#include "ModularGearBox.h"
#include "ModularVehicleData.h"
#include "ModularVehicleFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "PBDRigidsSolver.h"
#include "TerrainInteraction.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Physics/Experimental/PhysScene_Chaos.h"


DECLARE_CYCLE_STAT(TEXT("Modular Tick Component"), STAT_ModularTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Engine"), STAT_ModularEngine, STATGROUP_MovementPhysics);


//TODO : Throttle input rise
#define LOCTEXT_NAMESPACE "ModularMovement"

FORCEINLINE float OmegaToRPM(float Omega)
{
	return Omega * 30.f / PI;
}

FORCEINLINE float RPMToOmega(float RPM)
{
	return RPM * PI / 30.f;
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
	TArray<AActor*> Children;
	GetOwner()->GetAllChildActors(Children);
	for (const auto Actor : Children)
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (auto Casted = Cast<UModularWheel>(Component))
			{
				TempComponents.Add(Casted);
			}
		}
	}

	for (const auto Wheel : AdditionalWheels)
	{
		if (IsValid(Wheel))
		{
			if (!IsValid(Wheel->WheelState.WheelSetup))
			{
				Wheel->SetupWheels(this);
			}
			TempComponents.Add(Wheel);
		}
	}
	Components = TempComponents;

	auto Diffs=Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData;
	for(auto Wheel :Components)
	{
		if(Wheel->WheelState.ApplyDriveForce)
		{
			if(Diffs.IsValidIndex(Wheel->DifferentialIndex))
			{
				Diffs[Wheel->DifferentialIndex].Wheels.Add(Wheel);
			}
		}
	}
	Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData=Diffs;
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
	if (!GetSetup())
	{
		return 0;
	}
	return UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,
	                                           GetSetup()->GetIdleRPM(),
	                                           GetSetup()->GetMaxRPM(), 0, 1);
}

void UModularMovementComponent::HoldStarter( float StartTime)
{
	auto StaterFinish=[this,StartTime]()
	{
		// if no fuel keep the loop going else start engine 
		HoldStarter(VehicleState.CurrentFuel==0.f?StartTime: 0.f);
	};
	if(StartTime==0.f)
	{
		VehicleState.IsEngineOn=true;
		
		StarterTimerHandle.Invalidate();
		
		OnEngineStateChange.Broadcast(true,false);
		return;
		
	}
	GetWorld()->GetTimerManager().SetTimer(StarterTimerHandle,StaterFinish,StartTime,false);
	OnEngineStateChange.Broadcast(false,true);
	
}

void UModularMovementComponent::ReleaseStarter()
{
	if(StarterTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(StarterTimerHandle);
		OnEngineStateChange.Broadcast(VehicleState.IsEngineOn,false);
	}
}

void UModularMovementComponent::StopEngine()
{
	if(VehicleState.IsEngineOn)
	{
		VehicleState.IsEngineOn=false;
		OnEngineStateChange.Broadcast(false,false);
	}
}

void UModularMovementComponent::AddFuel(float Amount)
{
	VehicleState.CurrentFuel=FMath::Min(VehicleState.CurrentFuel+Amount,GetSetup()->GetTankCapacity());
}

void UModularMovementComponent::SetFuel(float Amount)
{
	VehicleState.CurrentFuel=FMath::Min(Amount,GetSetup()->GetTankCapacity());
}

float UModularMovementComponent::GetFuelRatio()
{
	if(!GetSetup())
	{
		return 0.f;
	}
	return VehicleState.CurrentFuel/ GetSetup()->GetTankCapacity();
}


void UModularMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	UMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return ShowSetupError("No Mesh Found at root component of the vehicle");
	}
	


	if (VehicleState.VehicleDataClass.LoadSynchronous())
	{
		VehicleState.VehicleData = NewObject<UBaseVehicleData>(this, VehicleState.VehicleDataClass.Get());
	}

	if (!GetSetup())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("Assign The Vehicle DataAsset "));
		UModularVehicleFunctionLibrary::NotifyError(
			"Assign The vehicle data by selecting modular movement component and setting the value in details panel");
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

if(ApplyRecommendedMeshProperties)
{
	MeshComponent->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	MeshComponent->BodyInstance.bSimulatePhysics = true;
	if (MeshComponent->GetCollisionObjectType())

	{
		MeshComponent->BodyInstance.bNotifyRigidBodyCollision = true;
	}
	MeshComponent->BodyInstance.bUseCCD = true;
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetCanEverAffectNavigation(false);
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
	

	if (ShouldProcessPhysics())
	{
		SteeringInput = CalcSteeringInput(DeltaTime);
		ThrottleInput = CalcThrottleInput(DeltaTime);
		BrakeInput = CalcBrakeInput();
		float WheelTorque = 0.f;

		UpdateAirDrag();
		UpdateEngine(fDeltaTime, WheelTorque);


		UpdateWheels(fDeltaTime, WheelTorque);

		UpdateReplicatedCosmeticData();
	}
	else
	{
		if (GetOwnerRole() < ROLE_AutonomousProxy)
		{
			for (UModularWheel* Component : Components)
			{
				if (Component->WheelState.WheelSetup)
				{
					Component->UpdateSteering(DeltaTime, this, SteeringInput);
					Component->UpdateSuspension(DeltaTime, this);
					Component->WheelState.AngularPosition += Component->WheelState.AngularVelocity * DeltaTime;


					float IntegerPart;
					Component->WheelState.AngularPosition = FMath::Modf(
						Component->WheelState.AngularPosition / (2 * PI), &IntegerPart) * (2 * PI);
				}
			}
		}
	}
}

void UModularMovementComponent::PreTick(FPhysScene_Chaos* Scene, float DeltaTime)
{
	if (AsyncCallBack == nullptr)
	{
		return;
	}

	FAsyncPhysicsInput* AsyncInput = AsyncCallBack->GetProducerInputData_External();

	if (AsyncInput == nullptr)
	{
		return;
	}

	UWorld* World = Scene->GetOwningWorld();

	if (World == nullptr)
	{
		return;
	}

	AsyncInput->Reset();

	AsyncInput->World = World;


	AsyncInput->Components.Add(this);
}

void UModularMovementComponent::PhysicsCallBack(float DeltaTime)
{
	VehicleTick(DeltaTime, nullptr);
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Check data 
	if (!GetSetup())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("Assign The Vehicle DataAsset "));
		UModularVehicleFunctionLibrary::NotifyError(
			"No Vehicle Data class in movement component. Please assign one in the details panel");
		PrimaryComponentTick.bCanEverTick = false;
		return;
	}


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

	if (ShouldProcessPhysics())
	{
		GetSetup()->GetGearBox()->Update(DeltaTime, this);
	}

	for (UModularWheel* Component : Components)
	{
		Component->UpdateAnimation(DeltaTime, this);
	}
	CaptureState(DeltaTime);


	// Check if we are in the process of body's state correction
	if (GetOwnerRole()<ROLE_Authority)
	{

		ApplyBodyInstanceData();
	}

	if(TerrainInteractionComponent)
	{
		TerrainInteractionComponent->Update(DeltaTime,this,Components);
	}
}

void UModularMovementComponent::BeginPlay()
{
	Super::BeginPlay();


	//Check data 
	if (!GetSetup())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("Assign The Vehicle DataAsset "));
		UModularVehicleFunctionLibrary::NotifyError(
			"No Vehicle Data class in movement component. Please assign one in the details panel");
		return;
	}

	GetWorld()->GetPhysicsScene()->OnPhysSceneStep.AddUObject(this, &UModularMovementComponent::PreTick);


	if (!AsyncCallBack)
	{
		AsyncCallBack = GetWorld()->GetPhysicsScene()->GetSolver()->CreateAndRegisterSimCallbackObject_External<
			FModularAsyncCallBack>();
	}

	GetSetup()->GetGearBox()->SetupGearBox();

	GetMesh()->SetSimulatePhysics(true);

	if (GetOwnerRole() < ROLE_AutonomousProxy)
	{
		GetMesh()->SetEnableGravity(false);
	}

	if(!VehicleState.IsEngineOn&&!SpawnWithTurnedOffEngine)
	{
		HoldStarter(0.f);
	}

	VehicleState.CurrentFuel=GetSetup()->GetTankCapacity();
	TerrainInteractionComponent=GetTerrainInteractionComponent();
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
	if(!VehicleState.IsEngineOn)
	{
		AxleRPM=0.f;
	}

	const float MaxRads=RPMToOmega(VehicleState.VehicleData->GetMaxRPM());
	const float MinRads=VehicleState.IsEngineOn?RPMToOmega(VehicleState.VehicleData->GetIdleRPM()):0.f;
	const float TargetRPM =GetSetup()->GetGearBox()->GetDriveRatio()==0?ThrottleInput*MaxRads: FMath::Clamp<float>(AxleRPM * GetSetup()->GetGearBox()->GetDriveRatio(),MinRads, MaxRads);

	VehicleState.EngineRads = FMath::FInterpConstantTo(VehicleState.EngineRads, TargetRPM, DeltaTime,0.1 * VehicleState.VehicleData->GetMaxRPM());
	

	VehicleState.CurrentRpm = OmegaToRPM(VehicleState.EngineRads);
	

	//TODO Refactor
	if (GetSetup()->ShouldZeroRpmWhenShifting() && GetSetup()->GetGearBox()->IsChangingGear())
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
		const float EngineTorque = VehicleState.IsEngineOn?ThrottleInput * GetSetup()->GetTorqueForRPM(VehicleState.CurrentRpm):0;
		
		//Gearbox 
		const float TransmissionTorque = GetSetup()->GetGearBox()->GetDriveRatio();
		

		//Fuel
		const float RPMRatio=GetRPMRatio();

		const float FuelConsumption=VehicleState.VehicleData->GetFuelConsumption(RPMRatio);

		if(VehicleState.IsEngineOn)
		{
			VehicleState.CurrentFuel=FMath::Max(0, VehicleState.CurrentFuel-FuelConsumption*DeltaTime);
			if(VehicleState.CurrentFuel==0.f)
			{
				StopEngine();
			}
		}

		


		WheelTorque = EngineTorque * TransmissionTorque;

		if (ModularVehicleDebugger)
		{
			//Set Torques and throttle
			ModularVehicleDebugger->EngineTorque = EngineTorque;
			ModularVehicleDebugger->WheelTorque = WheelTorque;
			ModularVehicleDebugger->ThrottleInput = ThrottleInput;
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


	const float UseSteeringValue = SteeringInput;
	

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

		if(RawThrottleInput==0.f&&RawSteeringInput==0.f)
		{
			VehicleState.TrackRight.TorqueTransfer=VehicleState.TrackLeft.TorqueTransfer=-1.f*BrakeInput*FMath::Sign(VehicleState.ForwardSpeed);
			
			
		}
		// In some rare cases where ground is perfectly flat and vehicle is perfectly still track forces can cancel each other so pr  that here
		if(FMath::Abs(VehicleState.ForwardSpeed)<10.f&&VehicleState.TrackRight.TorqueTransfer!=VehicleState.TrackLeft.TorqueTransfer)
		{
			if(VehicleState.TrackRight.TorqueTransfer<VehicleState.TrackLeft.TorqueTransfer)
			{
				VehicleState.TrackRight.TorqueTransfer=0;
				VehicleState.TrackLeft.TorqueTransfer=VehicleState.TrackLeft.TorqueTransfer*2;
			}else
			{
				VehicleState.TrackLeft.TorqueTransfer=0;
				VehicleState.TrackRight.TorqueTransfer=VehicleState.TrackRight.TorqueTransfer*2;
			}
			
		}
		
	}


	for (UModularWheel* Component : Components)
	{
		if (Component->WheelState.WheelSetup)
		{
			

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

			//calc and Apply Suspension forces 
			Component->UpdateSuspension(DeltaTime, this);
			//Apply Steering
			Component->UpdateSteering(DeltaTime, this, UseSteeringValue);
			if(!Component->WheelState.ApplyDriveForce||Component->DifferentialIndex==255)
			{
				Component->UpdateForces(DeltaTime,this);
			}
		}else
		{
			UModularVehicleFunctionLibrary::NotifyError(
				"Wheel Setup class is missing in wheel" + Component->GetName() + " . Please create and assign one !");
		}
	}


	float TempDiffRatio=-1.f;
	for(auto Diff:Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData)
	{
		if(!Diff.Wheels.IsEmpty())
		{
			if(TempDiffRatio<0.f)
			{
				//initialize
				TempDiffRatio=Diff.DifferentialRatio;
			}
			if(TempDiffRatio!=Diff.DifferentialRatio)
			{
				UE_LOG(LogModularVehicle,Error,TEXT("Found two active diffs with different ratios.This can cause unexpected behaviour"))
			}
			ApplyDifferential( Diff,WheelTorque*Diff.TorqueTransferRatio,DeltaTime);
		}
	}
	CurrentDifferentialRatio=TempDiffRatio;
	
}


EAIVehicleState UModularMovementComponent::DetermineAIState(float ForwardFactor, float DeltaTime)
{
	UWorld* World = GetWorld();
	const FVector VehicleLocation = GetOwner()->GetActorLocation() + FVector(
		0, 0, GetOwner()->GetRootComponent()->GetLocalBounds().BoxExtent.Z);
	FHitResult HitResultF;
	FHitResult HitResultB;
	const FVector VehicleDirection = GetMesh()->GetForwardVector();

	//trace
	if (World)
	{
		ActorsToIgnore.Add(GetOwner());


		const float TraceLenForward = GetSetup()->GetAITraceLength() + FMath::Max(0, VehicleState.ForwardSpeed) *
			VehicleState.VehicleData->GetAITraceSpeedMultiplier();
		const float TraceLenBackWard = GetSetup()->GetAITraceLength() + FMath::Min(0, VehicleState.ForwardSpeed) *
			VehicleState.VehicleData->GetAITraceSpeedMultiplier();
		DrawDebugString(GetWorld(), GetMesh()->GetComponentLocation(), FString::SanitizeFloat(TraceLenForward), nullptr,
		                FColor::Red, 0);
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

	if (VehicleState.AIState == Neutral)
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


	return GetOwner()->GetLocalRole() > ROLE_SimulatedProxy;
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

	HandBrakeInput = QHandbrakeInput == 1;
}


UTerrainInteraction* UModularMovementComponent::GetTerrainInteractionComponent()
{
	if(GetOwner())
	{
		auto Comp=	GetOwner()->GetComponentByClass(UTerrainInteraction::StaticClass());
		TerrainInteractionComponent=Cast<UTerrainInteraction>(Comp);
		
	}
	return TerrainInteractionComponent;
}

float UModularMovementComponent::CalcSteeringInput(float DeltaTime)
{
	// Determine the rate to use for interpolation
	const float InterpolationSpeed = (RawSteeringInput != 0.f || FMath::Sign(RawSteeringInput * SteeringInput) == 1
		                                  ? GetSetup()->GetSteerInputRise()
		                                  : GetSetup()->GetSteerInputFall());

	// Interpolate between the current steering input and the target
	SteeringInput = FMath::FInterpTo(SteeringInput, RawSteeringInput, DeltaTime, InterpolationSpeed);

	// Clamp the steering input to ensure it's within valid range
	SteeringInput = FMath::Clamp(SteeringInput, -1.0f, 1.0f);

	return SteeringInput;
}

float UModularMovementComponent::CalcBrakeInput() 
{
	float NewBrakeInput =VehicleState.IsEngineOn? 0.0f:GetSetup()->GetIdleBrakeInput();
	if (GetSetup()->ShouldReverseAsBrake())
	{
		

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
			if (FMath::Abs(VehicleState.ForwardSpeed) < GetSetup()->GetStopThreshold())
				
			{
				NewBrakeInput = 1.f;
			}
			else
			{
				NewBrakeInput = GetSetup()->GetIdleBrakeInput();
			
			}
		}

		NewBrakeInput= FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
	}else
	{
		if(RawBrakeInput==0&&RawThrottleInput<0)
		{
			NewBrakeInput=RawThrottleInput;
		}else
		{
			NewBrakeInput = FMath::Abs(RawBrakeInput);
		}
		
	}

	// if player isn't pressing forward or backwards...
	if (FMath::Abs(RawBrakeInput) < SMALL_NUMBER && FMath::Abs(RawThrottleInput) < SMALL_NUMBER)
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
	const bool IsInReverse=GetSetup()->GetGearBox()->IsInReverse();
	if (GetSetup()->ShouldReverseAsBrake())
	{
		if (RawBrakeInput > 0.f &&IsInReverse )
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
		//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if (RawThrottleInput > 0.f && IsInReverse || RawThrottleInput < 0.f&& !IsInReverse)
			{
				NewThrottleInput = 0.f;
			}
	}

	//Throttle and steer are not discrete in a  tank so we calculate both here
	if (GetSetup()->GetSteerType() == Tank)
	{
		NewThrottleInput = FMath::Clamp(FMath::Abs(RawThrottleInput) + FMath::Abs(SteeringInput), 0.f, 1.f);
	}

	if(!GetSetup()->ShouldReverseAsBrake()&&IsInReverse)
	{
			if(NewThrottleInput>0)
			{
				NewThrottleInput*=-1;
			}else
			{
				if(NewThrottleInput<0)
				{
					NewThrottleInput=0;
				}
			}
	}
	return NewThrottleInput;
}


void UModularMovementComponent::UpdateReplicatedCosmeticData()
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		return;
	}
	RepCosmeticData.EngineRPM = GetRPMRatio() * 255.f;
	RepCosmeticData.CurrentGear = GetSetup()->GetGearBox()->CurrentGear;
	RepCosmeticData.SteeringInput = SteeringInput;
	RepCosmeticData.CurrentFuel=VehicleState.CurrentFuel;
	RepCosmeticData.EngineOn=VehicleState.IsEngineOn;
	const auto BI = GetMesh()->GetBodyInstance();
	
	BI->GetRigidBodyState(RepCosmeticData.RigidBodyState);

	RepCosmeticData.WheelRepCosmeticDatas.Reset();
	for (int Index = 0; Index != Components.Num(); Index++)
	{
		RepCosmeticData.WheelRepCosmeticDatas.Insert(
			FWheelRepCosmeticData(Components[Index]->WheelState.TireStress,
			                      Components[Index]->WheelState.AngularVelocity), Index);
	}
}

void UModularMovementComponent::OnRep_RepCosmeticData()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		VehicleState.CurrentRpm = UKismetMathLibrary::MapRangeClamped(RepCosmeticData.EngineRPM, 0, 255,
		                                                              GetSetup()->GetIdleRPM(),
		                                                              GetSetup()->GetMaxRPM());

		if (const auto CurrentGear = GetSetup()->GetGearBox()->CurrentGear != RepCosmeticData.CurrentGear)
		{
			OnGearChange.Broadcast(CurrentGear, RepCosmeticData.CurrentGear, true);
			GetSetup()->GetGearBox()->SetCurrentGear(RepCosmeticData.CurrentGear);
		}
		SteeringInput = RepCosmeticData.SteeringInput;

		for (int Index = 0; Index != Components.Num(); Index++)
		{
			if (RepCosmeticData.WheelRepCosmeticDatas.IsValidIndex(Index))
			{
				Components[Index]->WheelState.TireStress = RepCosmeticData.WheelRepCosmeticDatas[Index].Slip;
				Components[Index]->WheelState.AngularVelocity = RepCosmeticData.WheelRepCosmeticDatas[Index].
					AngularVelocity;
			}
		}
	}

	VehicleState.CurrentFuel=RepCosmeticData.CurrentFuel;
	if(VehicleState.IsEngineOn!=RepCosmeticData.EngineOn)
	{
		VehicleState.IsEngineOn=RepCosmeticData.EngineOn;
		OnEngineStateChange.Broadcast(VehicleState.IsEngineOn,false);
	}
	
	NewestBodyInstance=RepCosmeticData.RigidBodyState;
	ApplyBodyInstanceData();
	
}

void UModularMovementComponent::ShowSetupError(FString Error)
{
	UE_LOG(LogModularVehicle, Error, TEXT("%s"), *Error)
#if WITH_EDITOR
	FMessageDialog::Open( EAppMsgType::Ok, FText::FromString(Error));
#endif
	
}

void UModularMovementComponent::ApplyBodyInstanceData() const
{
	

	if (GetMesh() == nullptr)
	{
		return ;
	}


	FBodyInstance* BI = GetMesh()->GetBodyInstance();
	if (BI && BI->IsInstanceSimulatingPhysics())
	{
		FRigidBodyState CurrentState;
		GetMesh()->GetRigidBodyState(CurrentState);
		
		const bool bShouldSleep = false;

		/////// POSITION CORRECTION ///////

		// Find out how much of a correction we are making
		const FVector DeltaPos = NewestBodyInstance.Position - CurrentState.Position;
		const float DeltaSize=DeltaPos.Size();
	

		// Snap position by default (big correction, or we are moving too slowly)
		FVector UpdatedPos = CurrentState.Position;
		FVector FixLinVel = FVector::ZeroVector;

		
		// If its a small correction and velocity is above threshold, only make a partial correction,
		// and calculate a velocity that would fix it over 'fixTime'.
		if (ErrorCorrection.MinDistanceToFix<DeltaSize&&DeltaSize<ErrorCorrection.MaxDistanceToFix)
		{
			const float LerpAlpha=UKismetMathLibrary::MapRangeClamped(DeltaSize,ErrorCorrection.MinDistanceToFix,ErrorCorrection.MaxDistanceToFix,0,ErrorCorrection.MaxAlpha);
			UpdatedPos = FMath::Lerp(CurrentState.Position, NewestBodyInstance.Position, LerpAlpha);
			FixLinVel = (NewestBodyInstance.Position - UpdatedPos) * ErrorCorrection.SpeedFactor;
			FixLinVel=FixLinVel.GetClampedToMaxSize(CurrentState.LinVel.Size());
			
		}else if(DeltaSize > ErrorCorrection.MaxDistanceToFix)
		{
			UpdatedPos=NewestBodyInstance.Position;
		}

		

		/////// ORIENTATION CORRECTION ///////
		// Get quaternion that takes us from old to new
		const FQuat InvCurrentQuat = CurrentState.Quaternion.Inverse();
		const FQuat DeltaQuat = NewestBodyInstance.Quaternion * InvCurrentQuat;

		FVector DeltaAxis(FVector::ZeroVector);
		float DeltaAng = 0.f; // radians
		DeltaQuat.ToAxisAndAngle(DeltaAxis, DeltaAng);
		DeltaAng = FMath::UnwindRadians(DeltaAng);

		// Snap rotation by default (big correction, or we are moving too slowly)
		FQuat UpdatedQuat = CurrentState.Quaternion;
		FVector FixAngVel = FVector::ZeroVector; // degrees per second

	
		// If the error is small, and we are moving, try to move smoothly to it
		if (ErrorCorrection.MinAngleToFix<FMath::Abs(DeltaAng)  &&FMath::Abs(DeltaAng) < ErrorCorrection.MaxAngleToFix)
		{
			const float LerpAlpha=UKismetMathLibrary::MapRangeClamped(FMath::Abs(DeltaAng),ErrorCorrection.MinAngleToFix,ErrorCorrection.MaxAngleToFix,0,ErrorCorrection.MaxAngularAlpha);
			UpdatedQuat = FMath::Lerp(CurrentState.Quaternion, NewestBodyInstance.Quaternion,LerpAlpha);
			FixAngVel = DeltaAxis.GetSafeNormal() * FMath::RadiansToDegrees(DeltaAng) * (1.f - LerpAlpha);
		
		}else if(FMath::Abs(DeltaAng) > ErrorCorrection.MaxAngleToFix)
		{
			UpdatedQuat=NewestBodyInstance.Quaternion;
		}

	

		/////// BODY UPDATE ///////
		BI->SetBodyTransform(FTransform(UpdatedQuat, UpdatedPos), ETeleportType::TeleportPhysics);
		BI->SetLinearVelocity(CurrentState.LinVel + FixLinVel, false);
	//	BI->SetAngularVelocityInRadians(FMath::DegreesToRadians(UpdatedQuat.Vector()), false);

		// state is restored when no velocity corrections are required
		bool bRestoredState = (FixLinVel.SizeSquared() < KINDA_SMALL_NUMBER) && (FixAngVel.SizeSquared() <
			KINDA_SMALL_NUMBER);
	

		/////// SLEEP UPDATE ///////
		const bool bIsAwake = BI->IsInstanceAwake();
		if (bIsAwake && (bShouldSleep && bRestoredState))
		{
			BI->PutInstanceToSleep();
		}
		else if (!bIsAwake)
		{
			BI->WakeInstance();
		}
	}


}


void UModularMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UModularMovementComponent, RepCosmeticData);
}



void UModularMovementComponent::ApplyDifferential( FDifferentialData DiffData, float EngineTorque, float DeltaTime)
{


	
	
	// Open differential: Equal torque distribution, but account for wheel slip
	float TotalAngularVelocity = 0.0f;
	float MinVelocity=BIG_NUMBER;
	float MaxVelocity=-BIG_NUMBER;
	const int WheelCount=DiffData.Wheels.Num();
	for (UModularWheel* Wheel : DiffData.Wheels)
	{
		const float AngularVelocity=Wheel->WheelState.AngularVelocity;
		TotalAngularVelocity += FMath::Abs(AngularVelocity);
		if(AngularVelocity<MinVelocity)
		{
			MinVelocity=AngularVelocity;
		}
		if(AngularVelocity>MaxVelocity)
		{
			MaxVelocity=AngularVelocity;
		}
	}

	const float Slip=DiffData.DifferentialType==Locked?1.f: UKismetMathLibrary::MapRangeClamped(FMath::Abs(MaxVelocity-MinVelocity),DiffData.MinSlip,DiffData.MaxSlip,0,1);
	switch (DiffData.DifferentialType)
	{

	case EModularDifferentialType::Simple:
		{
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				Wheel->SetDriveTorqueOnWheels(EngineTorque/WheelCount);
			}
		}
		break;
	case EModularDifferentialType::Open:
		{
			

			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				if (TotalAngularVelocity > SMALL_NUMBER)
				{
					const float SlipRatio = FMath::Abs(Wheel->WheelState.AngularVelocity) / TotalAngularVelocity;
					const float WheelTorque = SlipRatio * EngineTorque;
					Wheel->SetDriveTorqueOnWheels(WheelTorque);
				}
				else
				{
					// If all wheels are slipping, distribute torque equally
					const float TorquePerWheel = EngineTorque / WheelCount;
					Wheel->SetDriveTorqueOnWheels(TorquePerWheel);
				}
			}
			break;
		}
	case EModularDifferentialType::LimitedSlip:
		{
			
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				if (TotalAngularVelocity > SMALL_NUMBER)
				{
					const float SlipRatio = FMath::Abs(Wheel->WheelState.AngularVelocity) / TotalAngularVelocity;
					float LockedWheelTorque = EngineTorque /WheelCount;
					float WheelTorque = SlipRatio * EngineTorque;
					Wheel->SetDriveTorqueOnWheels(FMath::Lerp(WheelTorque,LockedWheelTorque,Slip));
				}
				else
				{
					// If all wheels are slipping, distribute torque equally
					const float TorquePerWheel = EngineTorque / WheelCount;
					Wheel->SetDriveTorqueOnWheels(TorquePerWheel);
				}
			}
		}
		break;
	case EModularDifferentialType::Locked:
		{
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				Wheel->SetDriveTorqueOnWheels(EngineTorque/WheelCount);
				
			}
		}
		break;
	default:
		UE_LOG(LogModularVehicle,Error,TEXT("Unimplemented differential Setup"));
	}

	
	float AverageSpeed=0.f;
	for (UModularWheel* Wheel : DiffData.Wheels)
	{
		//Apply those torques 
		Wheel->UpdateForces(DeltaTime, this);
		AverageSpeed+=Wheel->WheelState.AngularVelocity;
	}
	AverageSpeed=AverageSpeed/WheelCount;
	//Post Update

	

	
		for (UModularWheel* Wheel : DiffData.Wheels)
		{
			if(DiffData.DifferentialType==Locked||DiffData.DifferentialType==LimitedSlip)
			{
			Wheel->WheelState.AngularVelocity=FMath::Lerp(Wheel->WheelState.AngularVelocity,AverageSpeed,Slip) ;
			}
			const float MaxWheelAngularVelocity = (GetSetup()->GetMaxRPM() * GetSetup()->GetGearBox()->GetDriveRatio() * 2 * PI) / 60 * Wheel->WheelState.WheelSetup->WheelRadius / 100;;


			Wheel->WheelState.AngularVelocity=FMath::Clamp(Wheel->WheelState.AngularVelocity,-MaxWheelAngularVelocity,MaxWheelAngularVelocity);
	}

	
}


#undef LOCTEXT_NAMESPACE
