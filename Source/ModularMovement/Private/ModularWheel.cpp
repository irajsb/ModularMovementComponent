// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularMovement.h"
#include "WidgetComponent.h"
#include "VehicleDebugWidget.h"
#include "Kismet/KismetMathLibrary.h"



DECLARE_CYCLE_STAT(TEXT("Modular Updage Suspension"), STAT_ModularSuspension, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Modular Updage Forces"), STAT_ModularForces, STATGROUP_MovementPhysics);
 
// Sets default values for this component's properties
UModularWheel::UModularWheel()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UModularWheel::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UModularWheel::SetupWheels(UModularMovementComponent* ModularMovementComponent)
{
	WheelState.MovementComponent=ModularMovementComponent;
	const FTransform Transform=GetRelativeTransform();
	WheelState.InitialLocalLocation=Transform.GetLocation();//GetRelativeLocation();
	WheelState.InitialLocalRotation=Transform.GetRotation().Rotator();
	const float SideAngle=WheelState.InitialLocalLocation.Y<0?1:-1;;
	WheelState.SuspAngle=UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(this)*SideAngle;
	
	
	
}

void UModularWheel::UpdateSuspension(float DeltaTime,UModularMovementComponent* ModularMovementComponent)
{

	if(!ModularMovementComponent)
		return;
	MODULAR_CYCLE_COUNTER(STAT_ModularSuspension)
	//logic
	TArray<AActor*> ActorsToIgnore;
	WheelState.WheelLoad=FVector::ZeroVector;
	ActorsToIgnore.Add(GetOwner());
	const FTransform MeshTransform=ModularMovementComponent->GetMesh()->GetComponentTransform();
	const FVector ComponentLocation=MeshTransform.TransformPosition(WheelState.InitialLocalLocation+WheelState.WheelSetup->TraceStartOffset) ;
	
	//TODO
	const FVector DirectionVector=ModularMovementComponent->GetMesh()->GetUpVector();
	const FVector TraceEnd=ComponentLocation+(DirectionVector*-1*WheelState.WheelSetup->SuspensionLength);
	FHitResult TraceResult;
	TraceResult.TraceStart=ComponentLocation;
	TraceResult.TraceEnd=TraceEnd;
	TraceResult.bBlockingHit=false;
	
		TArray<FHitResult> Hits;
		bool ValidHitFound=false;

	bool Debug=false;
#if ! UE_BUILD_SHIPPING
	Debug=WheelState.WheelSetup->ShowSuspensionDebug;
#endif
	
		UKismetSystemLibrary::SphereTraceMulti(GetWorld(),ComponentLocation,TraceEnd,WheelState.WheelSetup->WheelRadius,ModularMovementComponent->GetSetup()->SuspensionTraceTypeQuery,true,ActorsToIgnore,Debug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,Hits,true);
	
		for(auto Hit : Hits)
		{
			if(Hit.bBlockingHit)
			{
			const FVector Position=	MeshTransform.InverseTransformPosition(Hit.ImpactPoint)-WheelState.InitialLocalLocation;
			if(FMath::Abs(Position.Y)<WheelState.WheelSetup->WheelWidth)
			{
				ValidHitFound=true;
				TraceResult=Hit;
				
				break;
				
				}
			
					
				
			}
		}
		if(!ValidHitFound)
		{
			
		}
	

		
	
	
		
	
	const float CurrentLen=FMath::Max<float>(0,TraceResult.Time);
	const float Stiffness=ModularMovementComponent->GetSpringStiffness(WheelState,1-CurrentLen);
	const float SuspDiff=FMath::Clamp(CurrentLen-WheelState.PreviousLen,-0.15f,0.15f);
	float DampingCorrection=0;
	if(CurrentLen-WheelState.PreviousLen<0)
	{
		
		DampingCorrection=-1*(((SuspDiff)*GetWheelSetup()->DampingCompress*Stiffness));
	}else
	{
		
		DampingCorrection=-1*(((SuspDiff)*GetWheelSetup()->DampingRebound*Stiffness));
	}

	if(TraceResult.bBlockingHit&&ModularMovementComponent->ShouldProcessPhysics())
	{
			const float AngleCorrection=(	FVector::DotProduct(TraceResult.ImpactNormal,	(TraceResult.TraceStart-TraceResult.TraceEnd).GetUnsafeNormal()));
			WheelState.WheelLoad=((AngleCorrection*FVector::UpVector*(Stiffness+DampingCorrection)));
			ModularMovementComponent->GetMesh()->GetBodyInstance()->AddForceAtPosition(WheelState.WheelLoad,TraceResult.TraceStart,false );
	}
	

	
	WheelState.PreviousLen=CurrentLen;
	WheelState.HitResult=TraceResult;

	
	//logic
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(WheelState.WheelSetup->ShowSuspensionDebug)
	{

		const FVector Force= FVector::UpVector*Stiffness;
		const FVector Damp= FVector::UpVector*DampingCorrection;
		
		DrawDebugSphere(GetWorld(),TraceResult.ImpactPoint,10,100,FColor::Red);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+50,(TraceResult.TraceStart+50)+Damp*0.0003,FColor::Green,false,-1,0,10);
		 FRotator WheelRot=	UKismetMathLibrary::ComposeRotators(GetForwardVector().Rotation(),FRotator(0,0,0));	
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+Force*0.0003,FColor::Red,false,-1,0,5);
		FVector WheelLocation=(TraceResult.bBlockingHit?TraceResult.ImpactPoint+(FVector(0,0,WheelState.WheelSetup->WheelRadius)):TraceResult.TraceEnd);
		DrawDebugCylinder(GetWorld(),WheelLocation+(UKismetMathLibrary::GetRightVector(WheelRot)*-1* WheelState.WheelSetup->WheelWidth/2),WheelLocation+(UKismetMathLibrary::GetRightVector(WheelRot)* WheelState.WheelSetup->WheelWidth/2),WheelState.WheelSetup->WheelRadius,20,TraceResult.bBlockingHit? FColor::Blue:FColor::Red,0,-1,2,0);
	

	}


	#endif

}

void UModularWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{
	MODULAR_CYCLE_COUNTER(STAT_ModularForces)
	if(!ModularMovementComponent)
		return;
	
// TODO Wheel Friction 
}

void UModularWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent,
	float InNormSteering)
{
	if(WheelState.WheelSetup->SteeringWheel)
	{
		const float AISteerMultiplier=ModularMovementComponent->VehicleState.IsAIVehicle?ModularMovementComponent->GetSetup()->AIMaxSteerMultiplier:1;
	/*if (FMath::Abs(GWheeledVehicleDebugParams.SteeringOverride) > 0.01f)
	{
	SteeringAngle = PWheel.Setup().WheelState.WheelSetup->SteeringMaxAngle * GWheeledVehicleDebugParams.SteeringOverride;
	}
	else*/
	{
		//

		const float WheelSide =WheelState.InitialLocalLocation.Y;
		
		float OutSteeringAngle = 0.f;

		switch (ModularMovementComponent->GetSetup()->SteerType)
		{
		case EModularSteerType::AngleRatio:
			{
				const bool OutsideWheel = (InNormSteering * WheelSide) > 0.f;
				OutSteeringAngle = InNormSteering * (OutsideWheel ? WheelState.WheelSetup->SteeringMaxAngle*AISteerMultiplier : WheelState.WheelSetup->SteeringMaxAngle*AISteerMultiplier *0.7/*TODO Setup().AngleRatio*/);

					
			}
			break;

		case EModularSteerType::Tank:
			{
				
				const float LeftTrackInput=InNormSteering;
				const float RightTrackInput=-InNormSteering;
				ModularMovementComponent-> VehicleState.TrackLeft.TorqueTransfer=0;
			ModularMovementComponent->	 VehicleState.TrackRight.TorqueTransfer=0;
				if (FMath::Abs(ModularMovementComponent->RawThrottleInput) > SMALL_NUMBER)
				{
					ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(ModularMovementComponent->RawThrottleInput)  + LeftTrackInput ;
					ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer = FMath::Abs(ModularMovementComponent->RawThrottleInput)  +RightTrackInput ;
				}
				else
				{
				
					ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer = FMath::Abs(ModularMovementComponent->RawThrottleInput)  + LeftTrackInput ;
					ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer = FMath::Abs(ModularMovementComponent->RawThrottleInput)  + RightTrackInput ;
					
				}
				
				if(WheelSide>0)
				{
					WheelState.TorqueTransferFactor=ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer;
				}else
				{
					WheelState.TorqueTransferFactor=ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer;
					
				}OutSteeringAngle=0;
				if(ModularMovementComponent-> GetSetup()->ShowInputProcessingDebug)
				{
					UE_LOG(LogModularVehicle,Warning,TEXT("Tank input Left %f Right %f"),ModularMovementComponent->VehicleState.TrackLeft.TorqueTransfer,ModularMovementComponent->VehicleState.TrackRight.TorqueTransfer);
				}
				}
			break;

		default:
        case EModularSteerType::SingleAngle:
			{
				
				OutSteeringAngle = WheelState.WheelSetup->SteeringMaxAngle * InNormSteering*AISteerMultiplier;
			}
			break;

		}

		
		//
		WheelState.SteerAngle=OutSteeringAngle*WheelState.WheelSetup->SteeringMultiplier;
	
	}

	
	}
}

void UModularWheel::SetDriveTorqueOnWheels(float Force)
{

	if(WheelState.WheelSetup->ApplyDriveForce){
	WheelState.DriveTorque=Force;
	}else{
	WheelState.DriveTorque=0;
	}
	
}

float UModularWheel::GetFastestWheelOmegaSpeed()
{
	if(WheelState.WheelSetup->ApplyDriveForce)
	{
		return WheelState.Omega;
	}return 0.0f;
	
}


void UModularWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent)
{

if(!AnimateChildComponent)
	return;
	FVector Location;
	FRotator Rotation;
	
	UModularVehicleFunctionLibrary::GetWheelAnimationData(this,Location,Rotation,DeltaTime);
	TArray<USceneComponent*> Components;
	GetChildrenComponents(false,Components);
	for(USceneComponent* Mesh : Components)
	{
		if(Mesh->IsA(UMeshComponent::StaticClass()))
		{
			if(1)
			{
				//Rotation.Pitch=Rotation.Pitch*-1;
				Rotation=(Rotation.Quaternion().Rotator());
			Mesh->SetRelativeRotation(Rotation) ;
			}else
			{
				SetRelativeRotation(Rotation) ;
			}

	
			Mesh->SetRelativeLocation(Location+FVector(0,0,WheelState.WheelSetup->WheelRadius));
		}
	}
	
	
}

FTransform UModularWheel::GetWheelTransform()
{return  GetComponentTransform();
}



void UModularWheel::UpdateWheelState(FWheelState In)
{WheelState=In;
}


FWheelState* UModularWheel::GetWheelState()
{
	return  &WheelState;
}

UModularVehicleWheelData* UModularWheel::GetWheelSetup() const
{
	return  WheelState.WheelSetup;
}

void UModularWheel::UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData)
{
	WheelState.WheelSetup=VehicleWheelData;
}

void UModularWheel::SetSteerOnWheel(float Angle)
{
	WheelState.SteerAngle=Angle;
}

void UModularWheel::AddDebugWidgetToWheel( TSubclassOf<UVehicleDebugWidget> Widget)
{
UWidgetComponent* WidgetComponent=	NewObject<UWidgetComponent>(this,UWidgetComponent::StaticClass(),NAME_None,RF_Transient);

	if(WidgetComponent)
	{
		
		WidgetComponent->RegisterComponent();
		WidgetComponent->SetWidgetClass(Widget);
		WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	}
	FAttachmentTransformRules TransformRules=FAttachmentTransformRules::KeepRelativeTransform;
	TransformRules.RotationRule=EAttachmentRule::KeepWorld;
	TransformRules.LocationRule=EAttachmentRule::SnapToTarget;
	WidgetComponent->AttachToComponent(this,TransformRules,NAME_None);
	Cast<UVehicleDebugWidget>(WidgetComponent->GetUserWidgetObject())->OwningWheel=this;	
}

