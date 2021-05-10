// Fill out your copyright notice in the Description page of Project Settings.


#include "SkeletalMeshWheel.h"
#include "ModularMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USkeletalMeshWheel::SetupWheels(UModularMovementComponent* ArcadeMovementComponent)
{
	const FTransform Transform=GetRelativeTransform();
	WheelState.InitialLocalLocation=Transform.GetLocation();//GetRelativeLocation();
	WheelState.InitialLocalRotation=Transform.GetRotation().Rotator();
	WheelState.MovementComponent=ArcadeMovementComponent;
}

void USkeletalMeshWheel::UpdateSuspension(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent)
{

	if(!ArcadeMovementComponent)
		return;

	ArcadeMovementComponent->WheelTrace(WheelState,DeltaTime,this);
}

void USkeletalMeshWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	if(!ArcadeMovementComponent)
		return;
	
	ArcadeMovementComponent->ApplyWheelForces(WheelState,DeltaTime,this);
}

void USkeletalMeshWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent,
	float /*TODO Change name */SteeringAngle)
{
	if(WheelState.WheelSetup->SteeringWheel)
	{
		ArcadeMovementComponent->CalculateSteeringAngle(WheelState,DeltaTime,this,SteeringAngle);
	}
}

void USkeletalMeshWheel::SetDriveTorqueOnWheels(float Force)
{

	if(WheelState.WheelSetup->ApplyDriveForce){
	WheelState.DriveTorque=Force;
	}else{
	WheelState.DriveTorque=0;
	}
	
}

float USkeletalMeshWheel::GetFastestWheelOmegaSpeed()
{
	if(WheelState.WheelSetup->ApplyDriveForce)
	{
		return WheelState.Omega;
	}return 0.0f;
	
}

int USkeletalMeshWheel::GetNumOfWheelsTouchingGround(bool OnlyDriveWheels)
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

void USkeletalMeshWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{

	
}

void USkeletalMeshWheel::SimulateWheelData(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	ArcadeMovementComponent->SimulateWheel(WheelState,DeltaTime,this);
}

FWheelState* USkeletalMeshWheel::GetWheelState()
{return  &WheelState;
}

