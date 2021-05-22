// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleStaticMeshWheel.h"
#include "ModularVehicleFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"

USimpleStaticMeshWheel::USimpleStaticMeshWheel()
{

}

void USimpleStaticMeshWheel::SetupWheels(UModularMovementComponent* ArcadeMovementComponent)
{
	const FTransform Transform=GetRelativeTransform();
	WheelState.InitialLocalLocation=Transform.GetLocation();//GetRelativeLocation();
	WheelState.InitialLocalRotation=Transform.GetRotation().Rotator();
	WheelState.MovementComponent=ArcadeMovementComponent;
}

void USimpleStaticMeshWheel::UpdateSuspension(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent)
{

	if(!ArcadeMovementComponent)
		return;

	ArcadeMovementComponent->WheelTrace(WheelState,DeltaTime,this);
}

void USimpleStaticMeshWheel::UpdateForces(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	if(!ArcadeMovementComponent)
		return;
	
	ArcadeMovementComponent->ApplyWheelForces(WheelState,DeltaTime,this);
}

void USimpleStaticMeshWheel::UpdateSteering(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent,
	float /*TODO Change name */SteeringAngle)
{
	if(WheelState.WheelSetup->SteeringWheel)
	{
		ArcadeMovementComponent->CalculateSteeringAngle(WheelState,DeltaTime,this,SteeringAngle);
	}
}

void USimpleStaticMeshWheel::SetDriveTorqueOnWheels(float Force)
{

	if(WheelState.WheelSetup->ApplyDriveForce){
	WheelState.DriveTorque=Force;
	}else{
	WheelState.DriveTorque=0;
	}
	
}

float USimpleStaticMeshWheel::GetFastestWheelOmegaSpeed()
{
	if(WheelState.WheelSetup->ApplyDriveForce)
	{
		return WheelState.Omega;
	}return 0.0f;
	
}

int USimpleStaticMeshWheel::GetNumOfWheelsTouchingGround(bool OnlyDriveWheels)
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

void USimpleStaticMeshWheel::UpdateAnimation(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{

	FVector Location;
	FRotator Rotation;
	
	UModularVehicleFunctionLibrary::GetWheelAnimationData(this,Location,Rotation);
	SetRelativeRotation(Rotation) ;
	SetRelativeLocation(Location);

	
}

void USimpleStaticMeshWheel::SimulateWheelData(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent)
{
	ArcadeMovementComponent->SimulateWheel(WheelState,DeltaTime,this);
}

FWheelState* USimpleStaticMeshWheel::GetWheelState()
{
	return  &WheelState;
}

