// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleStaticMeshWheel.h"

#include "ArcadePawn.h"
#include "Kismet/KismetSystemLibrary.h"

USimpleStaticMeshWheel::USimpleStaticMeshWheel()
{

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
