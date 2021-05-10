// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Interface.h"
#include "ModularVehicleWheelData.h"
#include "UObject/NoExportTypes.h"
#include "WheelInterface.generated.h"

/**
 * 
 */
class UModularMovementComponent;
UINTERFACE()
class TITANMOVEMENT_API UWheelInterface : public UInterface
{
	GENERATED_BODY()
	
};

class IWheelInterface
{

	GENERATED_BODY()

public:
	//called at init to gather essential data such as location relative to body
	virtual void SetupWheels(UModularMovementComponent* ArcadeMovementComponent){}
	virtual void UpdateSuspension(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent){};
	virtual void UpdateForces(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent){};
	virtual void UpdateSteering(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent,float SteeringAngle){};
	//Check driveable inside
	virtual void SetDriveTorqueOnWheels(float Force){};
	virtual float GetFastestWheelOmegaSpeed(){return  0.0;};
	virtual void UpdateAnimation(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent){};
	virtual void SimulateWheelData(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent){};
	virtual int GetNumOfWheelsTouchingGround(bool OnlyDriveWheels){return 0;};
	virtual FWheelState* GetWheelState(){return  nullptr;};

	//Index only for multi wheel components(Instanced Static Mesh)

	
};