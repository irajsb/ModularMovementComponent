//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "BaseVehicleData.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "ModularVehicleData.generated.h"

/**
 * 
 */

class UModularWheel;

UENUM()
enum EModularDifferentialType
{
	Simple,
	Open,
	LimitedSlip,
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
UCLASS()
class MODULARMOVEMENT_API UModularVehicleData : public UBaseVehicleData
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

	
	//How fast engine RPM can change . Used to stabilize RPM ranges from 0-1. Heavier vehicles generally have higher inertia due to heavier internal components 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,AdvancedDisplay)
	float EngineInertia=0.234;

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
	UPROPERTY(EditAnywhere, Category=Advanced)
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

//Getter Functions
	virtual float GetIdleRPM() const override;
	virtual float GetMaxRPM() const override;
	virtual bool ShouldZeroRpmWhenShifting() const override;
	virtual float GetTorqueForRPM(float RPM) const override;
	virtual float GetEngineInertia() const override;
	//Trans
	virtual UModularGearBox* GetGearBox()const  override;
	
	//misc
	virtual bool GetScaleTireFrictionWithSurfaceAngle() override;
	virtual float GetAirDragConstant() const override;
	virtual bool ShouldScaleDriveTorqueToNumberOfWheels() const override;
	virtual bool ShouldReverseAsBrake() const override;
	virtual float GetStopThreshold() const override;
	virtual float GetWrongDirectionThreshold() override;
	virtual TEnumAsByte<ETraceTypeQuery> GetSuspensionTraceTypeQuery() const override;
	virtual float GetReverseThreshold() const override;
	virtual float GetIdleBrakeInput() const override;
	//Steer
	virtual float GetSteerSpeedScaleForSpeed(float Speed)  override;
	virtual EModularSteerType GetSteerType() const override;
	virtual float GetSteerInputRise() const override;
	virtual float GetSteerInputFall() const override;

	virtual void GetAckermannValues(float& OutWheelBase, float& OutTrackWidth) override;
	virtual float GetCounterSteerMultiplier() const override;
	//AI
	virtual float GetAITraceLength() const override;
	virtual float GetAITraceSpeedMultiplier() const override;
	virtual float GetNearGoalDistance() override;
	virtual float GetDesireSpeedNearGoal() const override;
	virtual float GetDesireSpeedNormal() const override;
	virtual float GetDesireSpeedTurning() const override;
	virtual float GetDesireSpeedTurningAround() const override;
	virtual float GetFullThrottleSpeed() const override;
	virtual float GetAIMaxSteerMultiplier() const override;
	virtual float GetTurnThreshold() const override;

	//Fuel

	virtual float GetTankCapacity() const override;
	virtual float GetFuelConsumption(float RPMRatio) const override;

	virtual float GetSleepSlope() const override;

	virtual float GetSleepThreshold() const override;
};
