// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularVehicleFunctionLibrary.h"

#include "ModularMovementComponent.h"

float UModularVehicleFunctionLibrary::GetEngineRpm(UModularMovementComponent* MovementComponent)
{
return 	MovementComponent->VehicleState.CurrentRpm;
}

float UModularVehicleFunctionLibrary::GetEngineRpmRatio(UModularMovementComponent* MovementComponent)
{
	return 	MovementComponent->VehicleState.CurrentRpmRatio;
}

int UModularVehicleFunctionLibrary::GetCurrentGear(UModularMovementComponent* MovementComponent)
{
	return 	MovementComponent->VehicleState.CurrentGear;
}

int UModularVehicleFunctionLibrary::GetIdleGear(UModularMovementComponent* MovementComponent)
{
	return  MovementComponent->VehicleState.IdleGear;
}
