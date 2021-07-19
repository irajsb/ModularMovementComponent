// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"


#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */


class UModularMovementComponent;
UCLASS()
class MODULARMOVEMENT_API UModularVehicleWheelData : public UDataAsset
{
	GENERATED_BODY()
	public:

	
	//How much can wheels drop
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SuspensionLength=50;
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelRadius=30;
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelWidth=30;
	//Offset to apply to trace start
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector TraceStartOffset;
	//Force to apply for suspension N/m 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SpringRate=75304;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	bool ApplyDriveForce;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingRebound=8;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingCompress=5;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	bool SteeringWheel;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	float SteeringMaxAngle=30;
	//You can set this to -1 to allow back wheel steering 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steer)
	float SteeringMultiplier=1;

	//Suspension pivot point (rotates suspension when dropping used for off-road vehicles https://irajsb.github.io/ModularVehicleDocs/Pivot/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Suspension)
	 float SuspensionPivot;
	//Should we interpolate wheel location (0 to disable ) and whats the speed ?
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float AnimSpeed=0;





	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Debug)
	bool ShowSuspensionDebug;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Debug)
	bool ShowDrawFriction;



};


USTRUCT(BlueprintType)
struct FWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	UPROPERTY(BlueprintReadOnly)
	float PreviousLen;
	UPROPERTY(BlueprintReadWrite)
	float SteerAngle=0;
	UPROPERTY(BlueprintReadOnly)
	FHitResult HitResult;
	UPROPERTY(BlueprintReadWrite)
	bool bIsSlipping;
	//SuspensionForce That was applied;
	UPROPERTY(BlueprintReadOnly)
	FVector WheelLoad;
 	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UModularVehicleWheelData* WheelSetup;
	float DriveTorque;
	UPROPERTY(BlueprintReadWrite)
	float BrakeTorque;
	UPROPERTY(BlueprintReadOnly)
	float Spin;
	UPROPERTY(BlueprintReadOnly)
	bool Spinning;
	// [radians/sec] Wheel Rotation Angular Velocity
	UPROPERTY(BlueprintReadOnly)
	float Omega;
	UPROPERTY(BlueprintReadWrite)
	// [radians/sec] Wheel Rotation Angular Velocity
	float AngularPosition;// [radians]
	
	// Angle between wheel forwards and velocity vector
	UPROPERTY(BlueprintReadOnly)
	float SlipAngle;
	//Location of wheel relative to body
	UPROPERTY(BlueprintReadOnly)
	FVector InitialLocalLocation;
	UPROPERTY(BlueprintReadOnly)
	FRotator InitialLocalRotation;
	UPROPERTY(BlueprintReadOnly)
	float TorqueTransferFactor=1;
	UPROPERTY(BlueprintReadOnly)
	float SuspAngle;

	UPROPERTY(BlueprintReadOnly)
	UModularMovementComponent* MovementComponent;
	FVector PreviousLocation;
	UPROPERTY(BlueprintReadOnly)
	float CurrentPivotAngle;
	UPROPERTY(BlueprintReadOnly)
	float PreviousYaw;

	float AvailableGrip;
	bool WheelLocked;
	float SlipOmega;

	//Debug Data
#if ! UE_BUILD_SHIPPING
float LateralFrictionRatio;
float LongitudinalFrictionRatio;
#endif
	
};

 