// Fill out your copyright notice in the Description page of Project Settings.


#include "NoVisualWheel.h"
#include "ModularMovementComponent.h"
#include "ModularVehicleFunctionLibrary.h"
// Sets default values for this component's properties
UNoVisualWheel::UNoVisualWheel()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UNoVisualWheel::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UNoVisualWheel::SetupWheels(UModularMovementComponent* ArcadeMovementComponent)
{
	WheelState.MovementComponent=ArcadeMovementComponent;
	const FTransform Transform=GetRelativeTransform();
	WheelState.InitialLocalLocation=Transform.GetLocation();//GetRelativeLocation();
	WheelState.InitialLocalRotation=Transform.GetRotation().Rotator();
	const float SideAngle=WheelState.InitialLocalLocation.Y<0?1:-1;;
	WheelState.SuspAngle=UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(this)*SideAngle;
	
	
	
}

void UNoVisualWheel::UpdateSuspension(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent)
{

	if(!ArcadeMovementComponent)
		return;

	ArcadeMovementComponent->WheelTrace(WheelState,DeltaTime,this);
}

void UNoVisualWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	if(!ArcadeMovementComponent)
		return;
	
	ArcadeMovementComponent->ApplyWheelForces(WheelState,DeltaTime,this);
}

void UNoVisualWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent,
	float /*TODO Change name */SteeringAngle)
{
	if(WheelState.WheelSetup->SteeringWheel)
	{
		ArcadeMovementComponent->CalculateSteeringAngle(WheelState,DeltaTime,this,SteeringAngle);
	}
}

void UNoVisualWheel::SetDriveTorqueOnWheels(float Force)
{

	if(WheelState.WheelSetup->ApplyDriveForce){
	WheelState.DriveTorque=Force;
	}else{
	WheelState.DriveTorque=0;
	}
	
}

float UNoVisualWheel::GetFastestWheelOmegaSpeed()
{
	if(WheelState.WheelSetup->ApplyDriveForce)
	{
		return WheelState.Omega;
	}return 0.0f;
	
}

int UNoVisualWheel::GetNumOfWheelsTouchingGround(bool OnlyDriveWheels)
{
	if(OnlyDriveWheels)
	{
		if(WheelState.WheelSetup->ApplyDriveForce)
		{
			return WheelState.HitResult.bBlockingHit?1:0;
		}else
		{return 0;
		}
	}else
	{
		return WheelState.HitResult.bBlockingHit?1:0;
	}
	
	
}

void UNoVisualWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{


	
}

void UNoVisualWheel::SimulateWheelData(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	ArcadeMovementComponent->SimulateWheel(WheelState,DeltaTime,this);
}


FWheelState* UNoVisualWheel::GetWheelState()
{
	return  &WheelState;
}

