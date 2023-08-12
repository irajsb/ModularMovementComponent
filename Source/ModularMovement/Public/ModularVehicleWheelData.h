// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BaseTireModel.h"
#include "Engine.h"


#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum EWheelStatus
{
	Normal,
	Locked,
	Spinning 
};
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

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelMass=14;
	//Offset to apply to trace start
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector TraceStartOffset;
	
	//Force to apply for suspension N/m 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SpringRate=75000;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	bool ApplyDriveForce;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingRebound=8000;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingCompress=5000;
	
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float BrakeTorque=500;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float HandBrakeTorque=0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool ABS=true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool TractionControl=false;
	
	
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
	

	UPROPERTY(Instanced,EditAnywhere,BlueprintReadOnly)
	UBaseTireModel* TireModel;

};


USTRUCT(BlueprintType)
struct FWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	UPROPERTY(Transient)
	float PreviousLen;
	UPROPERTY(Transient)
	float SteerAngle=0.f;
	UPROPERTY(Transient)
	FHitResult HitResult;
	UPROPERTY(Transient)
	bool bIsSlipping;
	//SuspensionForce That was applied;
	UPROPERTY(Transient)
	FVector WheelLoad;
 	
	UPROPERTY(EditAnywhere)
	UModularVehicleWheelData* WheelSetup;
	UPROPERTY(Transient)
	float DriveTorque;
	UPROPERTY(Transient)
	float BrakeTorque;
	UPROPERTY(Transient)
	bool IsHandBrakeTorque;
	UPROPERTY(Transient)
	float Spin;

	
	float TireStress;

	// [radians/sec] Wheel Rotation Angular Velocity

	UPROPERTY(Transient)
	float AngularVelocity=0.f;
	UPROPERTY(Transient)
	float AngularAcceleration=0.f;
	UPROPERTY(Transient)
	// [radians/sec] Wheel Rotation Angular Velocity
	float AngularPosition;// [radians]
	
	// Angle between wheel forwards and velocity vector
	UPROPERTY(Transient)
	float SlipAngle;
	UPROPERTY(Transient)
	float SlipRatio;
	//Location of wheel relative to body
	UPROPERTY(Transient)
	FVector InitialLocalLocation;
	UPROPERTY(Transient)
	FRotator InitialLocalRotation;
	UPROPERTY(Transient)
	float TorqueTransferFactor=1;
	UPROPERTY(Transient)
	float SuspAngle;

	
	FVector PreviousLocation;
	UPROPERTY(Transient)
	float CurrentPivotAngle;
	UPROPERTY(Transient)
	float PreviousYaw;


	UPROPERTY(Transient)
	float DampingForce;
	TEnumAsByte<EWheelStatus> WheelStatus;

	//Debug Data
#if ! UE_BUILD_SHIPPING
float LateralFrictionRatio;
float LongitudinalFrictionRatio;
#endif
	
};

 