// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */


class UModularMovementComponent;
UCLASS()
class TITANMOVEMENT_API UModularVehicleWheelData : public UDataAsset
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
	//Force to apply
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float Stiffness;
	//amount of friction when moving forward
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float LongitudinalFrictionMultiplier=1.0;
	//amount of friction when moving Side ways
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float LateralFrictionMultiplier=1.0;
	//Modifier for lateral friction when wheels are locked (HandBrake)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SideSlipModifier=0.5;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	bool ApplyDriveForce;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	bool SteeringWheel;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	float SteeringMaxAngle=30;
	//TODo
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Brake)
	bool AffectedByHandBrake;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Brake)
	float BrakeTorque;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Brake)
	float HandBrakeTorque;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Brake)
	bool ABSEnabled;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Brake)
	bool TractionControlEnabled;
	//Suspension pivot point (rotates suspension when dropping used for off-road vehicles https://irajsb.github.io/ModularVehicleDocs/Pivot/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Suspension)
	 float SuspensionPivot;

	//Wheel Animation speed (just location not rotation)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float AnimSpeed;

	//1 by 1 chart x axis :(0-1) current compression of spring y axis(0-1) Spring Force multiplier in that given compression 
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FRuntimeFloatCurve SuspensionCurve;
};


USTRUCT(BlueprintType)
struct FWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	UPROPERTY(BlueprintReadOnly)
	float PreviousLen;
	UPROPERTY(BlueprintReadOnly)
	float SteerAngle=0;
	UPROPERTY(BlueprintReadOnly)
	FHitResult HitResult;
	UPROPERTY(BlueprintReadOnly)
	bool bIsSlipping;
	//SuspensionForce That was applied;
	UPROPERTY(BlueprintReadOnly)
	FVector WheelLoad;
 	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UModularVehicleWheelData* WheelSetup;
	float DriveTorque;
	UPROPERTY(BlueprintReadOnly)
	float BrakeTorque;
	UPROPERTY(BlueprintReadOnly)
	float Spin;
	UPROPERTY(BlueprintReadOnly)
	bool Spinning;
	// [radians/sec] Wheel Rotation Angular Velocity
	UPROPERTY(BlueprintReadOnly)
	float Omega;
	UPROPERTY(BlueprintReadOnly)
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
};

 