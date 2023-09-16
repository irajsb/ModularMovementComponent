//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "BaseTireModel.h"

#include "Engine/HitResult.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum ESuspensionType
{
	Sphere,
	Line,
};



class UModularMovementComponent;
UCLASS(Blueprintable,BlueprintType)
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

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	TEnumAsByte<ESuspensionType> SuspensionType;
	
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelWidth=30;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelMass=14;
	//Offset to apply to trace start
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	FVector TraceStartOffset;
	
	//Force to apply for suspension N/m 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SpringRate=75000;
	
	

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingRebound=8000;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingCompress=5000;
	
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float BrakeTorque=2000.0;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float HandBrakeTorque=2000.0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool ABS=true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool TractionControl=false;
	// Makes vehicle lose less power when going up hill . 0 is physically realistic 1 is no power loss
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float SteepSurfaceAssistance=0.5;
	
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	float SteeringMaxAngle=30;
	


	//Suspension pivot point (rotates suspension when dropping used for off-road vehicles https://irajsb.github.io/ModularVehicleDocs/Pivot/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Suspension)
	 float SuspensionPivot;
	
	//Should we interpolate wheel location (0 to disable ) and whats the speed ?
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float AnimSpeed=0;
	

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Instanced,Category = Friction)
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

	//Base class to create an instance from for setup of this wheel
	UPROPERTY(EditAnywhere,Category=Setup)
	TSoftClassPtr<UModularVehicleWheelData> WheelSetupClass;
	//Instanced setup which can be edited at runtime without conflicit
	UPROPERTY(BlueprintReadOnly,Transient,Category=Setup)
	UModularVehicleWheelData* WheelSetup=nullptr;
	//Automatically animate child component, Used for static mesh wheels
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool AnimateChildComponent;
	//Should this wheel apply drive force
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool ApplyDriveForce;
	//Steering scale for wheels can be negative to allow back wheel steering or less than 1 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	float SteerScale;
	//This wheel receives hand brake 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool AffectedByHandBrake=false;

	
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

	UPROPERTY(Transient)
	FVector PreviousLocation=FVector(0,0,0);
	UPROPERTY(Transient)
	float CurrentPivotAngle;


	UPROPERTY(Transient)
	float DampingForce;
	

	//Debug Data
#if ! UE_BUILD_SHIPPING
float LateralFrictionRatio;
float LongitudinalFrictionRatio;
#endif
	
};

 