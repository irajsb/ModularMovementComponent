//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "ModularGearBox.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ModularVehicleData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum  EModularSteerType 
{
	SingleAngle,
	Ackermann,
	Tank,
};




class UModularWheel;

UENUM()
enum EModularDifferentialType
{
	Simple,
	Open,	LimitedSlip,
	Locked
};

USTRUCT(BlueprintType)
struct FDifferentialData
{
	GENERATED_BODY()

	UPROPERTY(Category=Differential,EditAnywhere,BlueprintReadWrite)
	TEnumAsByte<EModularDifferentialType> DifferentialType=EModularDifferentialType::Simple;
	UPROPERTY(Category=Differential,EditAnywhere,BlueprintReadWrite,meta=(UIMax=1,UIMin=0.f,ClampMax=1.f,ClampMin=0.f))
	float TorqueTransferRatio=1.f;
	UPROPERTY(Category=Differential,EditAnywhere,BlueprintReadWrite)
	float DifferentialRatio=3.1f;

	//Difference in wheel speeds where diff clutch will start locking
	UPROPERTY(Category = Differential, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "DifferentialType == EModularDifferentialType::LimitedSlip"))
	float MinSlip=10.f;

	//Difference in wheel speeds where diff clutch is fully  locking
	UPROPERTY(Category = Differential, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "DifferentialType == EModularDifferentialType::LimitedSlip"))
	float MaxSlip=20.f;
	

	UPROPERTY()
	TArray<UModularWheel* > Wheels;
	
};
UCLASS(Blueprintable)
class MODULARMOVEMENT_API UModularVehicleData : public UObject
{
	GENERATED_BODY()
	public:
	UModularVehicleData();
	//Torque Curve Newton /Meter 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Essential)
	FRuntimeFloatCurve EngineTorqueCurve;
	//Torque Curve Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Essential)
	float TorqueMultiplier=1.f;
	//Set RPM TO Zero When Shifting
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	bool ZeroRpmWhenShifting;
	//Idle RPM Can be zero 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Essential)
	float IdleRpm;
	//Max rpm (Generally should be where torque curve reaches  0 torque  )
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Essential)
	float MaxRpm=6000;


	// List of all possible differential configs for this vehicle . Wheels can be moved between these differentials
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Differential)
	TArray<FDifferentialData> DifferentialData;

	
	//How fast engine RPM can change . Used to stabilize RPM . the bigger the number the faster it can change.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,AdvancedDisplay)
	float EngineInertia=0.1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,AdvancedDisplay)
	float EngineBrakeFactor=0.1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,AdvancedDisplay)
	float EngineMaxBrakeTorque=1000.f;

	//Gearbox Data
	UPROPERTY(Instanced,EditAnywhere,Category=Essential)
	UModularGearBox* GearBoxData;
	//Suspension Trace  trace 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
    TEnumAsByte<ETraceTypeQuery> SuspensionTraceTypeQuery;
	//Tire available grip will divided between drive wheels (Realistic value is: true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	bool ScaleTireFrictionWithSurfaceAngle=true;
	//Wheel divide drive torque to number of wheels touching the ground should be on to simulate a differential 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	bool ScaleDriveTorqueToNumberOfWheels;

	//steering Curve multiplies the steering value Time should be speed in KMH and Value should be Steering multiplier (ranges from 1-0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	FRuntimeFloatCurve SteerCurve;
	//Steering method
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	TEnumAsByte<EModularSteerType> SteerType;
	//Lateral distance between two wheels in same axel
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering,meta=(EditConditionHides,EditCondition="SteerType==EModularSteerType::Ackermann"))
	float TrackWidth=100.f;
	//Distance Between Two Axles
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering,meta=(EditConditionHides,EditCondition="SteerType==EModularSteerType::Ackermann"))
	float WheelBase=150.f;
	//How fast input should rise  Multiplied by DeltaTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float SteerInputRise=2;
	//How fast input should fall  Multiplied by DeltaTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float SteerInputFall=4;
	//Multiplier for input when vehicle is sliding and trying to reorient  itself
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float CounterSteerMultiplier=2.f;
	//Brake input will put vehicle to reverse once vehicle has stopped
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Advanced)
	bool bReverseAsBrake=true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Advanced)
	float SleepThreshold=10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Advanced)
	float SleepSlopeLimit=0.866f;;
	

	// Auto-brake when vehicle forward speed is opposite of player input by at least this much (cm/s)
	UPROPERTY(EditAnywhere, Category = Advanced)
	float WrongDirectionThreshold;
	// Auto-brake when absolute vehicle forward speed is less than this (cm/s)
	UPROPERTY(EditAnywhere, Category=Advanced)
	float StopThreshold;
	// How much to press the brake when the player has release throttle
	UPROPERTY(EditAnywhere, Category=Advanced,BlueprintReadWrite)
	float IdleBrakeInput;

	//Multiply steer limit angle by this value in order to make AI vehicles move easier
	UPROPERTY(EditAnywhere, Category=AI,meta = (ClampMin = "1.0", UIMin = "1.0", ClampMax = "2.0", UIMax = "2.0"))
	float AIMaxSteerMultiplier;
//1 to -1 (How much should dot product  between forward vector and destination should be in order to reverse ) 
	UPROPERTY(EditAnywhere, Category=AI,meta = (ClampMin = "-1.0", UIMin = "-1.0", ClampMax = "1.0", UIMax = "1.0"))
	float ReverseThreshold;
	//1 to -1 (How much should dot product  between forward vector and destination should be in order to steer slowly  )should be higher than  ReverseThreshold
	UPROPERTY(EditAnywhere, Category=AI,meta = (ClampMin = "-1.0", UIMin = "-1.0", ClampMax = "1.0", UIMax = "1.0"))
	float TurnThreshold;
	//Desired speed when moving forward normally kmh
	UPROPERTY(EditAnywhere, Category=AI)
	float DesireSpeedNormal;
	//Desired speed when towards a goal in side  (should be small to steer fully )
	UPROPERTY(EditAnywhere, Category=AI)
	float DesireSpeedTurning;
	//Desired speed when doing U Turn   kmh
	UPROPERTY(EditAnywhere, Category=AI)
	float DesireSpeedTurningAround;

	//desired speed final will be=MapRange(in,0,NearGoalDistance,DesiredSpeedNearGoal,NormalSpeed)  https://docs.unrealengine.com/en-US/BlueprintAPI/Math/Float/MapRangeClamped/index.html
	UPROPERTY(EditAnywhere, Category=AI)
	float DesireSpeedNearGoal;
	//distance to goal that we consider near goal to begin apply near goal speed in cm (unreal  units)
	UPROPERTY(EditAnywhere, Category=AI)
	float NearGoalDistance;
	//if difference between current speed and Desired speed was =FullThrottleSpeed we floor the gas 
	UPROPERTY(EditAnywhere, Category=AI)
	float FullThrottleSpeed;
	//avoidance trace length cm
	UPROPERTY(EditAnywhere, Category=AI)
	float AITraceLength;
	//how much should we increase trace len by speed (0.1 means for speed of 100KM/h we should trace 10M in front )trace len will not be lower than TraceLength property 
	UPROPERTY(EditAnywhere, Category=AI)
	float AITraceSpeedMultiplier;



	//For calculating air resistance unit : Meters^2
	UPROPERTY(EditAnywhere,Category=AirDrag,AdvancedDisplay)
	float VehicleFrontArea=2.2;
	//Air Density currently equal to planet earth air density 
	UPROPERTY(EditAnywhere,Category=AirDrag,AdvancedDisplay)
	float AirDensity=1.29;
	// The drag coefficient is a dimensionless quantity that is used to quantify the drag or resistance of an object in a fluid environment, such as air or water.
	UPROPERTY(EditAnywhere,Category=AirDrag,AdvancedDisplay)
	float AirDragCoefficient=0.3;
	
	

	//Fuel tank capacity in liters
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Fuel)
	float TankCapacity=70.f;
	//Fuel consumption per second at idle
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Fuel)
	float EngineFuelConsumptionIdle;
	//MaxRPM consumption per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Fuel)
	float EngineFuelConsumptionMaxRPM;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AntiRollover)
	bool bEnableAntiRollover;

	/** Anti rollover value threshold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AntiRollover, meta = (EditCondition = "bEnableAntiRollover"))
	float AntiRolloverValueThreshold;

	/** Relation between sine of Z axis delta angle and anti-rollover force applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AntiRollover, meta = (editcondition = "bEnableAntiRollover"))
	FRuntimeFloatCurve AntiRolloverForceCurve;

	

	
//engine
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetIdleRPM()const ;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetMaxRPM()const ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 bool ShouldZeroRpmWhenShifting()const;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetTorqueForRPM(float RPM)const ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetEngineInertia()const ;

	 float CalcEngineBrake(float ExcessRpm);;
	//Transmission

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 class UModularGearBox* GetGearBox()const ;



	

	//Misc
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetAirDragConstant()const ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 bool ShouldScaleDriveTorqueToNumberOfWheels() const ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	  bool ShouldReverseAsBrake()const;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetStopThreshold()const ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetWrongDirectionThreshold() ;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	  TEnumAsByte<ETraceTypeQuery> GetSuspensionTraceTypeQuery()const;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetReverseThreshold()const;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetIdleBrakeInput()const;

	
	//steer

	   float GetSteerSpeedScaleForSpeed(float Speed);;
	
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 EModularSteerType GetSteerType()const;

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetSteerInputRise()const;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetSteerInputFall()const;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	  void GetAckermannValues(float & OutWheelBase,float & OutTrackWidth);
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 float GetCounterSteerMultiplier()const;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 bool GetScaleTireFrictionWithSurfaceAngle();

	//Fuel

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Fuel)
	 float GetTankCapacity()const;
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Fuel)
	 float GetFuelConsumption(float RPMRatio)const;


	//AI
	 float GetAITraceLength()const;
	 float GetAITraceSpeedMultiplier()const;
	 float GetNearGoalDistance();;
	 float GetDesireSpeedNearGoal()const;
	 float GetDesireSpeedNormal()const;
	 float GetTurnThreshold()const;
	 float GetDesireSpeedTurning()const;
	 float GetDesireSpeedTurningAround()const;
	 float GetFullThrottleSpeed()const;
	 float GetAIMaxSteerMultiplier()const;
	 float GetSleepThreshold()const;
	FVector GetAntiRolloverTorque(float DeltaTime, FVector UpVector, float& LastAntiRolloverValue);
	float GetSleepSlope()const;
	
};
