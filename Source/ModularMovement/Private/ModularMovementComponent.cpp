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
#include "Async/Async.h"
#include "GameFramework/Pawn.h"
#include "PBDRigidsSolver.h"
#include "TimerManager.h"
#include "VehicleParticleSurfaceData.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
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

	auto Diffs = Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData;
	for (auto Wheel : Components)
	{
		if (Wheel->WheelState.ApplyDriveForce)
		{
			if (Diffs.IsValidIndex(Wheel->DifferentialIndex))
			{
				Diffs[Wheel->DifferentialIndex].Wheels.Add(Wheel);
			}
		}
	}
	Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData = Diffs;
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

void UModularMovementComponent::SetEngineHealth(float Input)
{
	VehicleHealth=Input;
	if(VehicleHealth==0.f&&VehicleState.IsEngineOn)
	{
		StopEngine();
	}

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

int UModularMovementComponent::GetNumberOfWheelsTouchingGround() const
{
	return VehicleState.WheelsOnGround;
}

float UModularMovementComponent::GetRPMRatio()
{
	if (!GetSetup())
	{
		return 0;
	}
	return UKismetMathLibrary::MapRangeClamped(VehicleState.CurrentRpm,
	                                           GetSetup()->IdleRpm,
	                                           GetSetup()->MaxRpm, 0, 1);
}

void UModularMovementComponent::HoldStarter(float StartTime)
{
	auto StaterFinish = [this,StartTime]()
	{
		// if no fuel keep the loop going else start engine 
		HoldStarter(VehicleState.CurrentFuel == 0.f ||VehicleHealth==0.f? StartTime : 0.f);
	};
	if (StartTime == 0.f)
	{
		VehicleState.IsEngineOn = true;
		ServerSetEngineOn(VehicleState.IsEngineOn );
		StarterTimerHandle.Invalidate();

		OnEngineStateChange.Broadcast(true, false);
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(StarterTimerHandle, StaterFinish, StartTime, false);
	OnEngineStateChange.Broadcast(false, true);
}

void UModularMovementComponent::ReleaseStarter()
{
	if (StarterTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(StarterTimerHandle);
		OnEngineStateChange.Broadcast(VehicleState.IsEngineOn, false);
	}
	
}

void UModularMovementComponent::StopEngine()
{
	if (VehicleState.IsEngineOn)
	{
		VehicleState.IsEngineOn = false;
		ServerSetEngineOn(VehicleState.IsEngineOn );
		OnEngineStateChange.Broadcast(false, false);
	}
}

void UModularMovementComponent::AddFuel(float Amount)
{
	VehicleState.CurrentFuel = FMath::Min(VehicleState.CurrentFuel + Amount, GetSetup()->TankCapacity);
}

void UModularMovementComponent::SetFuel(float Amount)
{
	VehicleState.CurrentFuel = FMath::Min(Amount, GetSetup()->TankCapacity);
}

float UModularMovementComponent::GetFuelRatio()
{
	if (!GetSetup())
	{
		return 0.f;
	}
	return VehicleState.CurrentFuel / GetSetup()->TankCapacity;
}


void UModularMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	UMeshComponent* MeshComponent = GetMesh();

	
	if (!MeshComponent)
	{
		return ShowSetupError("No Mesh Found at root component of the vehicle");
	}

	OriginalCOM= MeshComponent->GetCenterOfMass();

	OriginalDampening=FVector2D(MeshComponent->GetLinearDamping(),MeshComponent->GetAngularDamping());


	if (VehicleState.VehicleDataClass)
	{
		VehicleState.VehicleData = NewObject<UModularVehicleData>(this, VehicleState.VehicleDataClass.Get());
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

	if (ApplyRecommendedMeshProperties)
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
	AirDragConstant = GetSetup()->AirDragCoefficient;
	//0.5*GetSetup()->AirDragCoefficient*GetSetup()->VehicleFrontArea;
	RollingResistanceConstant = 30 * AirDragConstant;


	//Try Find Debugger
	ModularVehicleDebugger = Cast<UModularVehicleDebugger>(
		GetOwner()->GetComponentByClass(UModularVehicleDebugger::StaticClass()));
}

void UModularMovementComponent::VehicleTick(float DeltaTime, bool SubstepTick)
{


	MODULAR_CYCLE_COUNTER(STAT_ModularTickComponent)
	const float fDeltaTime = FMath::Min<float>(DeltaTime, 0.0633);

	// if not active skip sleep calcuate
	if(!IsActive())
	{
		return;
	}

	
	

	
	if(VehicleState.bSleeping)
	{
		for(const auto Comp:Components)
		{
			Comp->UpdateSteering(fDeltaTime, this, SteeringInput);
		}
		return;
	}
	if (ShouldProcessPhysics())
	{
		
		if (!IsTrailer)
		{
		

			
				UpdateAirDrag(GetMesh());
				
				UpdateEngine(fDeltaTime, VehicleState.WheelTorque);
				
			
		}
		GetMesh()->AddTorqueInRadians(GetSetup()->GetAntiRolloverTorque(fDeltaTime,GetMesh()->GetUpVector(),LastAntiRollover));
		UpdateWheels(fDeltaTime, VehicleState.WheelTorque);
	}
	else
	{
		if (ShouldProcessCosmetics())
		{
			for (UModularWheel* Component : Components)
			{
				if (Component->WheelState.WheelSetup)
				{
					Component->UpdateSteering(fDeltaTime, this, SteeringInput);
					Component->UpdateSuspension(fDeltaTime, this);
					
					Component->WheelState.AngularPosition += Component->WheelState.AngularVelocity * fDeltaTime;


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
	VehicleTick(DeltaTime, true);
}

void UModularMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	

		CaptureState(DeltaTime);

	const auto Processor= Cast<UVehicleInputProcessor>( InputProcessor->ClassDefaultObject);
	SteeringInput = Processor->CalcSteerInput(this,DeltaTime,RawSteeringInput);
	ThrottleInput = Processor->CalcThrottleInput(this,DeltaTime,RawThrottleInput,RawBrakeInput,RawSteeringInput);
	BrakeInput = Processor->CalcBrakeInput(this,DeltaTime,RawBrakeInput,RawThrottleInput);
	
	if(IsActive()&&!VehicleState.bSleeping)
	{
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
			UpdateReplicatedCosmeticData();
		}

	
	


		
	}
	for (UModularWheel* Component : Components)
	{
		Component->UpdateAnimation(DeltaTime, this);
	}

	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		if (GetOwnerRole() < ROLE_Authority)
		{
			ApplyBodyInstanceData();
		}
	}
	else
	{
		if (!IsLocal())
		{
			ApplyBodyInstanceData();
		}
		else
		{
			SetCosmeticDataOnServer(RepCosmeticData);
		}
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
			"No Vehicle Data class in movement component or . Please assign one in the details panel");
		return;
	}
	if(!GetSetup()->GetGearBox())
	{
		UE_LOG(LogModularVehicle, Error, TEXT("GearboxMissing"));
		UModularVehicleFunctionLibrary::NotifyError(
			"GearboxMissing");
		return;
	}

	GetMesh()->GetBodyInstance()->bGenerateWakeEvents=true;
	FScriptDelegate Delegate;
	
	GetMesh()->OnComponentWake.AddDynamic(this,&UModularMovementComponent::HandleComponentWake);
	GetWorld()->GetPhysicsScene()->OnPhysSceneStep.AddUObject(this, &UModularMovementComponent::PreTick);


	if (!AsyncCallBack)
	{
		AsyncCallBack = GetWorld()->GetPhysicsScene()->GetSolver()->CreateAndRegisterSimCallbackObject_External<
			FModularAsyncCallBack>();
	}

	GetSetup()->GetGearBox()->SetupGearBox();


	GetMesh()->SetSimulatePhysics(true);

	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		if (GetOwnerRole() < ROLE_AutonomousProxy)
		{
			GetMesh()->SetEnableGravity(false);
		}
	}
	else
	{
		if (!IsLocal())
		{
			GetMesh()->SetEnableGravity(false);
		}
	}

	if (!VehicleState.IsEngineOn && !SpawnWithTurnedOffEngine)
	{
		HoldStarter(0.f);
	}

	VehicleState.CurrentFuel = GetSetup()->TankCapacity;
	
}

void UModularMovementComponent::CaptureState(float DeltaTime)
{


	VehicleState.DriveWheelsOnGround =VehicleState.WheelsOnGround = 0;

	VehicleState.AxleRPM=0.f;
	//Get fastest wheel that is attached to engine 
	for (UModularWheel* Component : Components)
	{
		const float ComponentOmega = FMath::Abs(Component->GetFastestWheelOmegaSpeed());
		if (Component->GetWheelState()->HitResult.bBlockingHit )
		{
			VehicleState.WheelsOnGround++;
			if(Component->WheelState.ApplyDriveForce)
			{
				VehicleState.DriveWheelsOnGround++;
			}
		}
		if (ComponentOmega > VehicleState.AxleRPM)
		{
			VehicleState.AxleRPM = ComponentOmega;
		}
	}
	if (!VehicleState.IsEngineOn)
	{
		VehicleState.AxleRPM = 0.f;
	}
	
	VehicleState.ForwardSpeed = FVector::DotProduct(GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity(),
	                                                GetMesh()->GetForwardVector());

	VehicleState.SideSpeed = FVector::DotProduct(GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity(),
	                                             GetMesh()->GetRightVector());

	VehicleState.SlipAngle = FMath::Atan(
			VehicleState.SideSpeed / FMath::Abs(VehicleState.ForwardSpeed + 5.f/*Denominator*/));



	float BiggestSlip=-1.f;
	for(auto Comp: Components)
	{

		const float FSlip=FMath::Abs(Comp->GetWheelState()->SlipRatio);
		if(FSlip>BiggestSlip)
		{
			BiggestSlip = FSlip;
		}
	}
	VehicleState.WheelTraction=FMath::Min(BiggestSlip,1);
	VehicleState.WheelTraction=1.f-VehicleState.WheelTraction;
	
	if(AllowSleep)
	{
		if(GetSetup()->GetSteerType()!=Tank||AllowTankSleep){
		
			const float EffectiveThrottle=FMath::Min(RawThrottleInput,ClutchInput);
			// Wake if control input pressed
			if (VehicleState.bSleeping && (EffectiveThrottle!=0.f||RawSteeringInput!=0.f||(GetMesh()->IsAnyRigidBodyAwake()&&!AllowSleepUsingFriction)))
			{
			
				SetSleeping(false);
				UE_LOG(LogTemp,Log,TEXT("Sleep Off"))
				VehicleState.SleepTimer=0.f;
			}else if (VehicleState.WheelsOnGround==Components.Num() &&!VehicleState.bSleeping && RawThrottleInput==0.f &&  (GetMesh()->GetUpVector().Z > GetSetup()->SleepSlopeLimit))
			{
				if(VehicleState.SleepTimer>SleepDelay)
				{
					const float SpeedSqr = GetMesh()->GetPhysicsLinearVelocity().SizeSquared();
			
					const float SleepThreshold=GetSetup()->SleepThreshold;
					if (SpeedSqr < (SleepThreshold * SleepThreshold))
					{

						UE_LOG(LogTemp,Log,TEXT("Sleep On"))
						SetSleeping(true);
					
					}
				}else
				{
					VehicleState.SleepTimer+=DeltaTime;
				}
			}else
			{
				VehicleState.SleepTimer=0.f;
			}
		}
	}
}


void UModularMovementComponent::UpdateEngine(float DeltaTime, float& WheelTorque)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularEngine)




	const float MaxRads = RPMToOmega(VehicleState.VehicleData->GetMaxRPM());
	const bool EngineFree=GetSetup()->GetGearBox()->GetDriveRatio()==0.f||VehicleState.DriveWheelsOnGround==0;
	const float Clutch=EngineFree?0.f:ClutchInput;
	const float MinRads = VehicleState.IsEngineOn ? RPMToOmega(GetSetup()->IdleRpm) : 0.f;
	float AxleRPM=VehicleState.AxleRPM * GetSetup()->GetGearBox()->GetDriveRatio();
	if (GetSetup()->bUseGearboxRpm)
	{
		AxleRPM=GetSetup()->GetGearBox()->CurrentRpm;
		
	}
	const float DriveRPM=FMath::Lerp( ThrottleInput * MaxRads,AxleRPM,Clutch);
	const bool GearChange=GetSetup()->ZeroRpmWhenShifting && GetSetup()->GetGearBox()->IsChangingGear();
	 float TargetRPM =UKismetMathLibrary::MapRangeClamped(DriveRPM,0,MaxRads,MinRads,MaxRads);
	if(GearChange)
	{
		TargetRPM=0.f;
		WheelTorque=0.f;
	}
	
	VehicleState.EngineRads = FMath::FInterpConstantTo(VehicleState.EngineRads, TargetRPM, DeltaTime,
	                                                   VehicleState.VehicleData->GetEngineInertia() * VehicleState.VehicleData->GetMaxRPM());


	VehicleState.CurrentRpm = OmegaToRPM(VehicleState.EngineRads);


	
	if (!GearChange)
	{
		

		//Engine Torque 


		//Use curve 
		const float EngineTorque = VehicleState.IsEngineOn? TransientTorqueMultiplier*ThrottleInput * GetSetup()->GetTorqueForRPM(VehicleState.CurrentRpm): 0;
		//Engine braking 
		if(DriveRPM>MaxRads)
		{
			
		 VehicleState.EngineBrake=GetSetup()->CalcEngineBrake(OmegaToRPM( DriveRPM - MaxRads));
			
		}else
		{
			VehicleState.EngineBrake=0.f;
		}

		
		//Gearbox 
		const float TransmissionTorque = GetSetup()->GetGearBox()->GetDriveRatio();


		//Fuel
		const float RPMRatio = GetRPMRatio();

		const float FuelConsumption = VehicleState.VehicleData->GetFuelConsumption(RPMRatio);

		if (VehicleState.IsEngineOn)
		{
			VehicleState.CurrentFuel = FMath::Max(0, VehicleState.CurrentFuel - FuelConsumption * DeltaTime);
			if (VehicleState.CurrentFuel == 0.f)
			{
				StopEngine();
			}
		}


		WheelTorque = EngineTorque * TransmissionTorque*FMath::Max(VehicleHealth,GetSetup()->EngineBrokenMinTorqueFactor);
		VehicleState.EngineBrake=VehicleState.EngineBrake*TransmissionTorque;
	
		if (ModularVehicleDebugger)
		{
			//Set Torques and throttle
			ModularVehicleDebugger->EngineTorque = EngineTorque;
			ModularVehicleDebugger->WheelTorque = WheelTorque;
			ModularVehicleDebugger->ThrottleInput = ThrottleInput;
		}
	}
}

void UModularMovementComponent::UpdateAirDrag(UPrimitiveComponent * CompToApplyForceTo) 
{
	
	const float DragConstant=UseCustomDrag?CustomDragCoefficient:AirDragConstant;
	const FVector BodyVelocity = GetMesh()->GetBodyInstance()->GetUnrealWorldVelocity() / 100.f; //CM/s To Meter/s
	const FVector DragForce = BodyVelocity * BodyVelocity.Size() * DragConstant * -1;
	if(CompToApplyForceTo==GetMesh())
	{
		BodyForces+=SIForceToUnrealForce(DragForce);
	}else
	{
		// if custom body
		CompToApplyForceTo->AddForce(SIForceToUnrealForce(DragForce));
	}
	
}

void UModularMovementComponent::UpdateTankSteering(const float UseSteeringValue)
{
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

		if (RawThrottleInput == 0.f && RawSteeringInput == 0.f)
		{
			VehicleState.TrackRight.TorqueTransfer = VehicleState.TrackLeft.TorqueTransfer = -1.f * BrakeInput *
				FMath::Sign(VehicleState.ForwardSpeed);
		}
		// In some rare cases where ground is perfectly flat and vehicle is perfectly still track forces can cancel each other so pr  that here
		if (FMath::Abs(VehicleState.ForwardSpeed) < 10.f && VehicleState.TrackRight.TorqueTransfer != VehicleState.
			TrackLeft.TorqueTransfer)
		{
			if (VehicleState.TrackRight.TorqueTransfer < VehicleState.TrackLeft.TorqueTransfer)
			{
				VehicleState.TrackRight.TorqueTransfer = 0;
				VehicleState.TrackLeft.TorqueTransfer = VehicleState.TrackLeft.TorqueTransfer * 2;
			}
			else
			{
				VehicleState.TrackLeft.TorqueTransfer = 0;
				VehicleState.TrackRight.TorqueTransfer = VehicleState.TrackRight.TorqueTransfer * 2;
			}
		}
	}

	
	
}

void UModularMovementComponent::UpdateWheels(float DeltaTime, float WheelTorque)
{
	//Capturing inputs
	//Steer


	const float UseSteeringValue = SteeringInput;
	
	UpdateTankSteering(UseSteeringValue);

	float TempDiffRatio = -1.f;
	for (auto Diff : Cast<UModularVehicleData>(VehicleState.VehicleData)->DifferentialData)
	{
		if (!Diff.Wheels.IsEmpty())
		{
			if (TempDiffRatio < 0.f)
			{
				//initialize
				TempDiffRatio = Diff.DifferentialRatio;
			}
			if (TempDiffRatio != Diff.DifferentialRatio)
			{
				UE_LOG(LogModularVehicle, Error,
					   TEXT("Found two active diffs with different ratios.This can cause unexpected behaviour"))
			}
			ApplyDifferential(Diff, WheelTorque * Diff.TorqueTransferRatio, DeltaTime);
		}
	}
	CurrentDifferentialRatio = TempDiffRatio;

	ParallelFor(Components.Num(), [&](int32 Index)
	{
		UModularWheel* Component = Components[Index];
		if (Component->WheelState.WheelSetup)
		{
			Component->WheelState.BrakeTorque =VehicleState.EngineBrake+ BrakeInput * Component->WheelState.WheelSetup->BrakeTorque;
			
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

			
			// Calc and Apply Suspension forces 
			Component->UpdateSuspension(DeltaTime, this);
			// Apply Steering
			Component->UpdateSteering(DeltaTime, this, UseSteeringValue);
			Component->UpdateForces(DeltaTime, this);
		}
		else
		{
			// Note: Using a lambda to capture 'this' for the NotifyError function
			AsyncTask(ENamedThreads::GameThread, [this, ComponentName = Component->GetName()]()
			{
				UModularVehicleFunctionLibrary::NotifyError(
					"Wheel Setup class is missing in wheel" + ComponentName + ". Please create and assign one!");
			});
		}
	});


	// Apply accumulated forces
	GetMesh()->AddForce(BodyForces);
	BodyForces=FVector::ZeroVector;
	for (const auto Wheel:Components)
	{
		Wheel->ApplyAccumulatedForces();
	}


}


EAIVehicleState UModularMovementComponent::DetermineAIState(float ForwardFactor, float DeltaTime)
{
	const UWorld* World = GetWorld();
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
		const bool AIDebug = true;
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
		if (ForwardFactor < GetSetup()->ReverseThreshold)
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


bool UModularMovementComponent::ShouldProcessPhysics()
{
	if(CachedShouldProcessPhysics.IsSet())
	{
		
		return CachedShouldProcessPhysics.GetValue();
	}
	if (GetNetMode() == NM_Standalone)
	{
		CachedShouldProcessPhysics=true;
		return true;
	}
	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		CachedShouldProcessPhysics= GetOwner()->GetLocalRole() > ROLE_SimulatedProxy;
		return CachedShouldProcessPhysics.GetValue();
		
	}
	CachedShouldProcessPhysics= IsLocal();
	return CachedShouldProcessPhysics.GetValue();
}

bool UModularMovementComponent::ShouldProcessCosmetics()
{
	if(CachedShouldProcessCosmetics.IsSet())
	{
		return CachedShouldProcessCosmetics.GetValue();
	}
	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		CachedShouldProcessCosmetics= GetOwnerRole() < ROLE_AutonomousProxy;
		return CachedShouldProcessCosmetics.GetValue();
	}
	
	CachedShouldProcessCosmetics= !IsLocal();
	return CachedShouldProcessCosmetics.GetValue();
	
}

bool UModularMovementComponent::ShouldReplicateInput() const
{
	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		return (GetPawnOwner()->GetLocalRole() != ROLE_Authority && GetPawnOwner()->IsLocallyControlled());
	}
	return false;
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






void UModularMovementComponent::UpdateReplicatedCosmeticData()
{
	if (NetworkMode == EVehicleNetworkMode::Default)
	{
		if (GetOwnerRole() < ROLE_Authority)
		{
			return;
		}
	}
	RepCosmeticData.EngineRPM = GetRPMRatio() * 255.f;
	RepCosmeticData.CurrentGear = GetSetup()->GetGearBox()->CurrentGear;
	RepCosmeticData.SteeringInput = SteeringInput;
	RepCosmeticData.CurrentFuel = VehicleState.CurrentFuel;
	RepCosmeticData.EngineOn = VehicleState.IsEngineOn;
	RepCosmeticData.IsSleep=VehicleState.bSleeping;
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
	CosmeticDataInitialized = true;
	if (NetworkMode == EVehicleNetworkMode::ClientAuthoritative && IsLocal())
	{
		return;
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy || NetworkMode == EVehicleNetworkMode::ClientAuthoritative)
	{
		VehicleState.CurrentRpm = UKismetMathLibrary::MapRangeClamped(RepCosmeticData.EngineRPM, 0, 255,
		                                                              GetSetup()->IdleRpm,
		                                                              GetSetup()->MaxRpm);

		if (const auto CurrentGear = GetSetup()->GetGearBox()->CurrentGear != RepCosmeticData.CurrentGear)
		{
			OnGearChange.Broadcast(CurrentGear, RepCosmeticData.CurrentGear, true);
			if (!GetSetup()->GetGearBox()->IsManual||GetOwnerRole()==ROLE_AutonomousProxy)
			{
				GetSetup()->GetGearBox()->SetCurrentGear(RepCosmeticData.CurrentGear);
			}
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

	VehicleState.CurrentFuel = RepCosmeticData.CurrentFuel;
	if (VehicleState.IsEngineOn != RepCosmeticData.EngineOn)
	{
		VehicleState.IsEngineOn = RepCosmeticData.EngineOn;
		OnEngineStateChange.Broadcast(VehicleState.IsEngineOn, false);
	}

	NewestBodyInstance = RepCosmeticData.RigidBodyState;
	if (!IsLocal())
	{
		ApplyBodyInstanceData();
	}
}

void UModularMovementComponent::ServerSetEngineOn_Implementation(bool NewOn)
{
	VehicleState.IsEngineOn = NewOn;
}

void UModularMovementComponent::SetCosmeticDataOnServer_Implementation(FRepCosmeticData Data)
{
	RepCosmeticData = Data;
	OnRep_RepCosmeticData();
}

void UModularMovementComponent::ShowSetupError(const FString& Error)
{
	UE_LOG(LogModularVehicle, Error, TEXT("%s"), *Error)
#if WITH_EDITOR
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error));
#endif
}

void UModularMovementComponent::ApplyBodyInstanceData() 
{
	
	
	if (GetMesh() == nullptr || !CosmeticDataInitialized)
	{
		return;
	}
	

		if (GetOwnerRole() < ROLE_Authority)
		{
			if (GetOwnerRole()==ROLE_AutonomousProxy)
			{
				
				NewestBodyInstance.Position.Y=2930.806086;
				
			}else
			{
				NewestBodyInstance.Position.Y=2035.994077;
			}
		}
	
	//DrawDebugSphere(GetWorld(),NewestBodyInstance.Position,250,20,FColor::White,false,-1,1);

	FBodyInstance* BI = GetMesh()->GetBodyInstance();
	FRigidBodyState CurrentState;
	GetMesh()->GetRigidBodyState(CurrentState);

	if (GetOwnerRole()==ROLE_SimulatedProxy)
	{
		CurrentState=NewestBodyInstance;
	}
	
	FVector UpdatedPos = CurrentState.Position;
	FQuat UpdatedQuat = CurrentState.Quaternion;
	FVector FixAngVel = FVector::ZeroVector; // degrees per second
	FVector FixLinVel = FVector::ZeroVector;
	if (BI && BI->IsInstanceSimulatingPhysics())
	{
		if (GetOwnerRole()>ROLE_SimulatedProxy)
		{
			

			/////// POSITION CORRECTION ///////

			// Find out how much of a correction we are making
			const FVector DeltaPos = NewestBodyInstance.Position - CurrentState.Position;
			const float DeltaSize=DeltaPos.Size();
	

			// Snap position by EVehicleNetworkMode::Default (big correction, or we are moving too slowly)
			

		
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

			// Snap rotation by EVehicleNetworkMode::Default (big correction, or we are moving too slowly)
			

	
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
		}

	

		/////// BODY UPDATE ///////
		BI->SetBodyTransform(FTransform(UpdatedQuat, UpdatedPos), ETeleportType::TeleportPhysics);
		BI->SetLinearVelocity(CurrentState.LinVel + FixLinVel, false);
		BI->SetAngularVelocityInRadians(FMath::DegreesToRadians(UpdatedQuat.Vector()), false);

		// state is restored when no velocity corrections are required
		const bool bRestoredState = (FixLinVel.SizeSquared() < KINDA_SMALL_NUMBER) && (FixAngVel.SizeSquared() <
			KINDA_SMALL_NUMBER);
	

		
		/////// SLEEP UPDATE ///////
		
		if ( (RepCosmeticData.IsSleep!=VehicleState.bSleeping &&( bRestoredState||VehicleState.bSleeping)))
		{
			SetSleeping(RepCosmeticData.IsSleep);
		}
	

	}


}

void UModularMovementComponent::SetCanSleep(bool Input)
{
	if(VehicleState.bSleeping&&!Input)
	{
		SetSleeping(false);
		
	}
	AllowSleep=Input;
}

bool UModularMovementComponent::IsLocal()
{
	// Check if the cached result is available
	if (CachedIsLocal.IsSet())
	{
		return CachedIsLocal.GetValue();
	}

	const ENetMode NetMode = GetNetMode();
	const auto Owner = GetOwner();
	if (NetMode == NM_Standalone)
	{
		// Not networked.
	
		CachedIsLocal = true;
		return true;
	}

	if (NetMode == NM_Client && Owner->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Networked client in control.
		CachedIsLocal = true;
		return true;
	}

	if (NetMode == NM_ListenServer &&static_cast<AController*>( GetWorld()->GetFirstPlayerController()) == GetPawnOwner()->GetController())
	{
		CachedIsLocal = true;
		
		return true;
	}

	// If none of the conditions are met, the result is false
	CachedIsLocal = true;
	return false;
}

float UModularMovementComponent::GetMassPerWheel() const
{
	return GetMesh()->GetMass()/Components.Num();
}

void UModularMovementComponent::SetSleepOnBody(UPrimitiveComponent* PrimitiveComponent, bool Sleep)
{
	if(const auto SK=Cast<USkeletalMeshComponent>(PrimitiveComponent))
	{
		for(const auto Body:SK->Bodies)
		{
			
			if(Sleep)
			{
				Body->PutInstanceToSleep();
			}else
			{
				Body->WakeInstance();
			}
		}
	}else
	{
		if(const auto SM=Cast<UStaticMeshComponent>(PrimitiveComponent))
		{
			auto Body=	SM->BodyInstance;
			if(Sleep)
			{
				Body.PutInstanceToSleep();
			}else
			{
				Body.WakeInstance();
			}
		}
	}
}

void UModularMovementComponent::ApplyAirbornePhysics()
{
	const auto Setup=GetSetup();
	if(Setup->ApplyAirbornePhysics)
	{

		if(GetNumberOfWheelsTouchingGround()==0)
		{
			GetMesh()->SetCenterOfMass(Setup->AirborneCOM);
			GetMesh()->SetLinearDamping(Setup->AirborneDampening.X);
			GetMesh()->SetLinearDamping(Setup->AirborneDampening.Y);
		}else
		{
			GetMesh()->SetCenterOfMass(OriginalCOM);
			GetMesh()->SetLinearDamping(OriginalDampening.X);
			GetMesh()->SetLinearDamping(OriginalDampening.Y);
		}

		
	}
}

bool UModularMovementComponent::IsInReverse() const
{
	if(GetSetup())
	{
		if(const auto Gearbox=GetSetup()->GetGearBox())
		{
			return Gearbox->CurrentGear<Gearbox->IdleGear;
		}
	}
	return false;
}


void UModularMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UModularMovementComponent, RepCosmeticData);
}


void UModularMovementComponent::ApplyDifferential(FDifferentialData DiffData, float EngineTorque, float DeltaTime) const
{
	// Open differential: Equal torque distribution, but account for wheel slip
	float TotalAngularVelocity = 0.0f;
	float MinVelocity = BIG_NUMBER;
	float MaxVelocity = -BIG_NUMBER;

	EngineTorque=EngineTorque*ClutchInput;
	const int WheelCount = DiffData.Wheels.Num();
	for (const UModularWheel* Wheel : DiffData.Wheels)
	{
		const float AngularVelocity = Wheel->WheelState.AngularVelocity;
		TotalAngularVelocity += FMath::Abs(AngularVelocity);
		if (AngularVelocity < MinVelocity)
		{
			MinVelocity = AngularVelocity;
		}
		if (AngularVelocity > MaxVelocity)
		{
			MaxVelocity = AngularVelocity;
		}
	}

	const float Slip = DiffData.DifferentialType == Locked
		                   ? 1.f
		                   : UKismetMathLibrary::MapRangeClamped(FMath::Abs(MaxVelocity - MinVelocity),
		                                                         DiffData.MinSlip, DiffData.MaxSlip, 0, 1);
	switch (DiffData.DifferentialType)
	{
	case Simple:
		{
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				Wheel->SetDriveTorqueOnWheels(EngineTorque / WheelCount);
			}
		}
		break;
	case Open:
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
	case LimitedSlip:
		{
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				if (TotalAngularVelocity > SMALL_NUMBER)
				{
					const float SlipRatio = FMath::Abs(Wheel->WheelState.AngularVelocity) / TotalAngularVelocity;
					float LockedWheelTorque = EngineTorque / WheelCount;
					float WheelTorque = SlipRatio * EngineTorque;
					Wheel->SetDriveTorqueOnWheels(FMath::Lerp(WheelTorque, LockedWheelTorque, Slip));
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
	case Locked:
		{
			for (UModularWheel* Wheel : DiffData.Wheels)
			{
				Wheel->SetDriveTorqueOnWheels(EngineTorque / WheelCount);
			}
		}
		break;
	 default:
		UE_LOG(LogModularVehicle, Error, TEXT("Unimplemented differential Setup"));
	
	}


	float AverageSpeed = 0.f;
	for (const UModularWheel* Wheel : DiffData.Wheels)
	{
		
		AverageSpeed += Wheel->WheelState.AngularVelocity;
	}
	AverageSpeed = AverageSpeed / WheelCount;
	//Post Update


	// Pre-calculate constants if they don't change frequently
	const float MaxRPM = GetSetup()->MaxRpm;
	const float DriveRatio = GetSetup()->GetGearBox()->GetDriveRatio();
	constexpr float RPMToRadPerSec = (2.0f * PI) / 60.0f;

	for (UModularWheel* Wheel : DiffData.Wheels)
	{
		if (DiffData.DifferentialType == Locked || DiffData.DifferentialType == LimitedSlip)
		{
			// Adjust angular velocity based on differential type
			Wheel->WheelState.AngularVelocity = FMath::Lerp(Wheel->WheelState.AngularVelocity, AverageSpeed, Slip);
		}

		// Calculate max angular velocity for this wheel
		const float MaxWheelAngularVelocity = MaxRPM * DriveRatio * RPMToRadPerSec * 
			Wheel->WheelState.WheelSetup->WheelRadius / 100.0f;

		// Clamp the angular velocity to prevent unrealistic values
		if(MaxWheelAngularVelocity!=0.f)
		{
			Wheel->WheelState.AngularVelocity = FMath::Clamp(Wheel->WheelState.AngularVelocity, 
															 -MaxWheelAngularVelocity,
															 MaxWheelAngularVelocity);
		}
	}

	
}



void UModularMovementComponent::SetSleeping(bool bEnableSleep)
{
	VehicleState.bSleeping=bEnableSleep;
	VehicleState.CurrentRpm=VehicleState.IsEngineOn? GetSetup()->IdleRpm:0.f;
	if(!AllowSleepUsingFriction)
	{
		
		if(bEnableSleep)
		{
			if(GetSetup())
			{
				if(GetSetup()->GetSteerType()!=Tank)
				{
					if(GetSetup()->GetGearBox())
					{
						GetSetup()->GetGearBox()->SetToIdle();
					}
				}
			}
		}
	
		for (const auto Comp:Components)
		{
			Comp->WheelState.AngularVelocity=0.f;
		}
		
		SetSleepOnBody(GetMesh(),bEnableSleep);
	
		for(const auto Comp:Components)
		{
			if(Comp->ConstraintParent)
			{
			
				SetSleepOnBody(Comp->ConstraintParent,bEnableSleep);
				SetSleepOnBody(Comp->WheelCollision,bEnableSleep);
			
			
			}
			if(Comp->SurfaceData)
			{
				Comp->SurfaceData->OnSleep();
			}
		}
		TArray<UActorComponent*> Comps;
		GetOwner()->GetComponents(Comps,true);
		for(const auto Comp:Comps)
		{
			if(const auto Prim=Cast<UPrimitiveComponent>(Comp))
				SetSleepOnBody(Prim,bEnableSleep);
		}
	
	}else
	{

		for(auto Comp :GetWheels())
		{
			
			Comp->WheelCollision->SetPhysMaterialOverride(bEnableSleep?Comp->FullFrictionDefaultPhysMaterial:Comp->NoFrictionDefaultPhysMaterial);
			
				Comp->WheelState.AngularVelocity=0.f;
			
	
			if(Comp->SurfaceData)
			{
				Comp->SurfaceData->OnSleep();
			}
		}
	}
	OnSleepChange.Broadcast(bEnableSleep);
}


#undef LOCTEXT_NAMESPACE




