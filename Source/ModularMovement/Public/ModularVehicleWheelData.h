//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "BaseTireModel.h"
#include "DefaultTireModel.h"

#include "Engine/HitResult.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleWheelData.generated.h"

/**
 * 
 */

class UDefaultTireModel;

UENUM(BlueprintType)
enum ESuspensionType
{
	Sphere,
	Line,
	Constraint
};



class UModularMovementComponent;
UCLASS(Blueprintable,BlueprintType)
class MODULARMOVEMENT_API UModularVehicleWheelData : public UDataAsset
{
	GENERATED_BODY()
	UModularVehicleWheelData()
	{
		TireModel=CreateDefaultSubobject<UDefaultTireModel>("DefaultTire");
	}
	public:

	
	//How much can wheels drop in cm
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SuspensionLength=50;
	
	//trace wheel radius in cm
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelRadius=30;

	// Trace channel
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	TEnumAsByte<ESuspensionType> SuspensionType;
	// Suspension force in other directions rather than Z . This is necessary to allow natural twists of suspension
	UPROPERTY(EditAnywhere,AdvancedDisplay,BlueprintReadWrite,Category=Essential,meta=(EditCondition="SuspensionType==Constraint",EditConditionHides))
	float NonZForceMultiplier=5.f;
	//to allow solid axle drop there should be move limit in other axis
	UPROPERTY(EditAnywhere,AdvancedDisplay,BlueprintReadWrite,Category=Essential,meta=(EditCondition="SuspensionType==Constraint",EditConditionHides))
	float XYAxisMoveLimit=5.f;
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelWidth=30;
	// Wheel mass . Changes inertia and how fast the wheel speed changes. you can use this to stablizie the wheel if needed
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float WheelMass=50;
	//Offset to apply to trace start 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	FVector TraceStartOffset;
	
	//Force to apply for suspension N/m 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float SpringRate=75000;
	
	
	//Damping for when spring is being extended from compress N.s/M
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingRebound=4000;
	//Damping for when spring is being compressed  N.s/M
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingCompress=2500;
	
	//Brake torque
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float BrakeTorque=4000.0;

	//Hand brake torque  applied to wheels that have this enabled
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float HandBrakeTorque=4000.0;
	// Prevent wheels locking while braking and try to maintain perfect grip
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool ABS=true;
	//Prevent engine from spinning the wheels by reducing power and trying to keep perfect grip
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	bool TractionControl=false;
	// Makes vehicle lose less power when going up hill . 0 is physically realistic 1 is no power loss
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Friction)
	float SteepSurfaceAssistance=0.0;
	
	//Angle in degrees
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Steer)
	float SteeringMaxAngle=30;
	
	
	//Suspension pivot point (rotates suspension when dropping used for off-road vehicles 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Suspension)
	 float SuspensionPivot;
	
	//Smooth the wheel location and rotation 0 for disable 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Essential)
	float AnimSpeed=0;
	
	//Tire model implementation
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Instanced,Category = Friction)
	UBaseTireModel* TireModel;

};


USTRUCT(BlueprintType)
struct FModularWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	UPROPERTY(Transient)
	float PreviousLen=0.f;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float SteerAngle=0.f;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	FHitResult HitResult=FHitResult();
	UPROPERTY(Transient)
	bool bIsSlipping=false;
	//SuspensionForce That was applied;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	FVector WheelLoad=FVector::ZeroVector;

	//Base class to create an instance from for setup of this wheel
	UPROPERTY(EditAnywhere,Category=Setup)
	TSoftClassPtr<UModularVehicleWheelData> WheelSetupClass;
	//Instanced setup which can be edited at runtime without conflicit
	UPROPERTY(BlueprintReadOnly,Transient,Category=Setup)
	UModularVehicleWheelData* WheelSetup=nullptr;
	//Automatically animate child component, Used for static mesh wheels
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool AnimateChildComponent=false;
	//Should this wheel apply drive force
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool ApplyDriveForce=true;
	//Steering scale for wheels can be negative to allow back wheel steering or less than 1 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	float SteerScale=0.f;
	//This wheel receives hand brake 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool AffectedByHandBrake=false;

	
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float DriveTorque=0.f;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float BrakeTorque=0.f;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	bool IsHandBrakeTorque=false;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float Spin=0.f;

	
	float TireStress=0.f;

	// [radians/sec] Wheel Rotation Angular Velocity

	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float AngularVelocity=0.f;
	
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float AngularAcceleration=0.f;
	
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	// [radians/sec] Wheel Rotation Angular Velocity
	float AngularPosition=0.f;// [radians]
	
	// Angle between wheel forwards and velocity vector
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float SlipAngle=0.f;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	float SlipRatio=0.f;
	//Location of wheel relative to body
	UPROPERTY(Transient)
	FVector InitialLocalLocation=FVector::ZeroVector;
	UPROPERTY(Transient)
	FRotator InitialLocalRotation=FRotator::ZeroRotator;
	UPROPERTY(Transient)
	float TorqueTransferFactor=1.f;
	UPROPERTY(Transient)
	float SuspAngle=0.f;

	UPROPERTY(Transient)
	FVector PreviousLocation=FVector(0,0,0);
	UPROPERTY(Transient)
	float CurrentPivotAngle=0.f;

	
	UPROPERTY(Transient)
	float DampingForce=0.f;
	

	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	FTransform LastAnimTransform;
	UPROPERTY(Transient,BlueprintReadOnly,Category="WheelState")
	FTransform CurrentAnimTransform;
	
};

 