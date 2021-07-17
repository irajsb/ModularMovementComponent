// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleData.generated.h"

/**
 * 
 */
 
UENUM(BlueprintType)
enum  EModularSteerType 
{
	SingleAngle,
    AngleRatio,
    Tank,
};

UENUM(BlueprintType)
enum EVehicleNetworkMode
{	ClientPredictive,
	ServerAuthoritative,
	ClientAuthoritative,
	 
};
 USTRUCT(BlueprintType)
 struct FModularGearInfo
 {
 	GENERATED_USTRUCT_BODY()
 	//This Gear's Ratio
 	UPROPERTY(EditAnywhere,BlueprintReadWrite)
 	float GearRatio;
 	/** Value of engineRevs/maxEngineRevs that is low enough to gear down */
 	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
 	float DownRatio;
 
 	/** Value of engineRevs/maxEngineRevs that is high enough to gear up */
 	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
 	float UpRatio;
 	FModularGearInfo():GearRatio(1.0f),DownRatio(0.1),UpRatio(1.f)
 	{
 		
 	};
 	FModularGearInfo(float Ratio):GearRatio(Ratio),DownRatio(0.1),UpRatio(1.f)
 	{
 	}
 };
UCLASS()
class MODULARMOVEMENT_API UModularVehicleData : public UDataAsset
{
	GENERATED_BODY()
	public:
	UModularVehicleData();
	//Torque curve
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float ConstantTorque;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,meta=(EditCondition="ConstantTorque==0.0"))
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Transmission)
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
	float TraceLength;
	//how much should we increase trace len by speed (0.1 means for speed of 100KM/h we should trace 10M in front )trace len will not be lower than TraceLength property 
	UPROPERTY(EditAnywhere, Category=AI)
	float TraceSpeedMultiplier;





	UPROPERTY(EditAnywhere, Category=Debug)
	bool ShowGearboxDebug;
	UPROPERTY(EditAnywhere, Category=Debug)
	bool ShowInputProcessingDebug;
	UPROPERTY(EditAnywhere, Category=Debug)
	bool AIDebug;




	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Network)
	TEnumAsByte<EVehicleNetworkMode> NetworkMode;
	


};
