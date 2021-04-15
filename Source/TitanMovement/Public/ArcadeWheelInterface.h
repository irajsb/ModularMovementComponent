// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "UObject/NoExportTypes.h"
#include "ArcadeWheelInterface.generated.h"

/**
 * 
 */
class UArcadeMovementComponent;
UINTERFACE()
class TITANMOVEMENT_API UArcadeWheelInterface : public UInterface
{
	GENERATED_BODY()
	
};

class IArcadeWheelInterface
{

	GENERATED_BODY()

public:
	//called at init to gather essential data such as location relative to body
	virtual void SetupWheels(UArcadeMovementComponent* ArcadeMovementComponent){}
	virtual void UpdateSuspension(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent){};
	virtual void UpdateForces(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent){};
	virtual void UpdateSteering(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent,float SteeringAngle){};
	//Check driveable inside
	virtual void SetDriveTorqueOnWheels(float Force){};
	virtual float GetFastestWheelOmegaSpeed(){return  0.0;};
	//Index only for multi wheel components(Instanced Static Mesh)

	
};