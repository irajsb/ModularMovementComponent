// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BaseVehicleData.h"
#include "Engine.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleData.generated.h"

/**
 * 
 */
 

UCLASS()
class MODULARMOVEMENT_API UModularVehicleData : public UBaseVehicleData
{
	GENERATED_BODY()
	public:
	UModularVehicleData();
	//Torque Curve Newton /Meter 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	FRuntimeFloatCurve EngineTorqueCurve;
	//SetRPMTOZeroWhenShifting
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	bool ZeroRpmWhenShifting;
	//Idle RPM
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float IdleRpm;
	//Max rpm 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float MaxRpm=7500;
	// Normalized 0-1 how much of energy is wasted in transmission  1 =ideal full efficient ?
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Transmission, Meta=( UIMin="0", UIMax="1", ClampMin="0.0", ClampMax="1.0"))
	float TransmissionEfficiency=1;
	/*Affects rpm calculation ,tweak if rpm is not matching your expectations*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Transmission)
	float DifferentialRatio=1.0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Transmission)
	float GearChangeTime=0.5;
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category=Transmission)
    TArray<FModularGearInfo> Gears;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
    TEnumAsByte<ETraceTypeQuery> SuspensionTraceTypeQuery;

	//Wheel divide drive torque to number of wheels touching the ground useful for switching between AWD 4WD RWD 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	bool ScaleDriveTorqueToNumberOfWheels;

	//steering Curve multiplies the steering value Time should be speed in KMH and Value should be Steering multiplier (ranges from 1-0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	FRuntimeFloatCurve SteerCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	TEnumAsByte<EModularSteerType> SteerType;
	//How fast input should rise  Multiplied by DeltaTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float SteerInputRise=5;
	//How fast input should fall  Multiplied by DeltaTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float SteerInputFall=20;

	//Animation speed when steering has been released
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	float SteeringAnimationSpeed=120;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bReverseAsBrake=true;
	

	// Auto-brake when vehicle forward speed is opposite of player input by at least this much (cm/s)
	UPROPERTY(EditAnywhere, Category = Advanced)
	float WrongDirectionThreshold;
	// Auto-brake when absolute vehicle forward speed is less than this (cm/s)
	UPROPERTY(EditAnywhere, Category=Advanced)
	float StopThreshold;
	// How much to press the brake when the player has release throttle
	UPROPERTY(EditAnywhere, Category=Advanced)
	float IdleBrakeInput;

	//Multiply steer limit angle by this value in order to make AI vahicles move easier
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Network)
	TEnumAsByte<EVehicleNetworkMode> NetworkMode;
	

//Getter Functions
	virtual float GetIdleRPM() const override;
	virtual float GetMaxRPM() const override;
	virtual bool ShouldZeroRpmWhenShifting() const override;
	virtual float GetTorqueForRPM(float RPM) const override;
	//Trans
	virtual TArray<FModularGearInfo> GetGears() const override;
	virtual float GetGearChangeTime() const override;
	virtual float GetDifferentialRatio() const override;
	virtual float GetTransmissionEfficiency() const override;
	//misc
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
	virtual float GetSteeringAnimationSpeed() const override;
	virtual EVehicleNetworkMode GetNetworkMode() const override;
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
};
