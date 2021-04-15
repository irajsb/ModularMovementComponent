// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleStaticMeshWheel.h"

#include "ArcadePawn.h"
#include "Kismet/KismetSystemLibrary.h"

USimpleStaticMeshWheel::USimpleStaticMeshWheel()
{

}

void USimpleStaticMeshWheel::SetupWheels(UArcadeMovementComponent* ArcadeMovementComponent)
{
	WheelState.LocalLocation=GetRelativeLocation();
}

void USimpleStaticMeshWheel::UpdateSuspension(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent)
{

	if(!ArcadeMovementComponent)
		return;

	ArcadeMovementComponent->WheelTrace(WheelState,DeltaTime,this);
}

void USimpleStaticMeshWheel::UpdateForces(float DeltaTime, UArcadeMovementComponent* ArcadeMovementComponent)
{
	if(!ArcadeMovementComponent)
		return;
	if(WheelState.WheelSetup->ApplyDriveForce)
	ArcadeMovementComponent->ApplyWheelForces(WheelState,DeltaTime,this);
}

void USimpleStaticMeshWheel::UpdateSteering(float DeltaTime, UArcadeMovementComponent* ArcadeMovementComponent,
	float /*TODO Change name */SteeringAngle)
{
	if(WheelState.WheelSetup->SteeringWheel)
	{
		ArcadeMovementComponent->CalculateSteeringAngle(WheelState,DeltaTime,this,SteeringAngle);
	}
}

void USimpleStaticMeshWheel::SetDriveTorqueOnWheels(float Force)
{

	
	WheelState.DriveTorque=Force;
}

float USimpleStaticMeshWheel::GetFastestWheelOmegaSpeed()
{
	return WheelState.Omega;
}
