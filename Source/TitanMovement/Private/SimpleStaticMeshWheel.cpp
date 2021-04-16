// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleStaticMeshWheel.h"

#include "ArcadePawn.h"
#include "Kismet/KismetMathLibrary.h"

USimpleStaticMeshWheel::USimpleStaticMeshWheel()
{

}

void USimpleStaticMeshWheel::SetupWheels(UArcadeMovementComponent* ArcadeMovementComponent)
{
	WheelState.InitialLocalLocation=GetRelativeLocation();
	WheelState.InitialLocalRotation=GetRelativeRotation();
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

	if(WheelState.WheelSetup->ApplyDriveForce){
	WheelState.DriveTorque=Force;
	}else{
	WheelState.DriveTorque=0;
	}
	
}

float USimpleStaticMeshWheel::GetFastestWheelOmegaSpeed()
{
	return WheelState.Omega;
}

void USimpleStaticMeshWheel::UpdateAnimation(float DeltaTime, UArcadeMovementComponent* ArcadeMovementComponent)
{

	SetRelativeRotation(UKismetMathLibrary::ComposeRotators(WheelState.InitialLocalRotation,FRotator(FMath::RadiansToDegrees(-1*WheelState.AngularPosition),WheelState.SteerAngle,0))) ;
	SetRelativeLocation(WheelState.InitialLocalLocation-FVector(0,0,(WheelState.HitResult.Time)*WheelState.WheelSetup->SuspensionLength));
	
}
