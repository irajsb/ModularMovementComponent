// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */



UCLASS()
class TITANMOVEMENT_API UModularVehicleWheelData : public UDataAsset
{
	GENERATED_BODY()
	public:
	//How much can wheels drop
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float SuspensionLength=50;
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float WheelRadius=30;
	
	//Offset to apply to trace start
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector TraceStartOffset;
	//Force to apply
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float Stiffness;
	//amount of friction when moving forward
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float LongitudinalFrictionMultiplier=1.0;
	//amount of friction when moving Side ways
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float LateralFrictionMultiplier=1.0;
	//Modifier for lateral friction when wheels are locked (HandBrake)
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float SideSlipModifier=0.5;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool ApplyDriveForce;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool SteeringWheel;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float SteeringMaxAngle=30;
	//TODo
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool AffectedByHandBrake;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float BrakeTorque;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float HandBrakeTorque;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool ABSEnabled;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool TractionControlEnabled;
	
	
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
};

 