// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ModularVehicleData.generated.h"

/**
 * 
 */
 
UENUM(BlueprintType)
enum  EArcadeSteerType 
{
	SingleAngle,
    AngleRatio,
    Ackermann,
};


 USTRUCT(BlueprintType)
 struct FArcadeGearInfo
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
 	FArcadeGearInfo():GearRatio(1.0f),DownRatio(0.1),UpRatio(1.f)
 	{
 		
 	};
 	FArcadeGearInfo(float Ratio):GearRatio(Ratio),DownRatio(0.1),UpRatio(1.f)
 	{
 	}
 };
UCLASS()
class TITANMOVEMENT_API UModularVehicleData : public UDataAsset
{
	GENERATED_BODY()
	public:
	UModularVehicleData();
	//Torque curve
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float ConstantTorque;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine,meta=(EditCondition="ConstantTorque==0.0"))
	FRuntimeFloatCurve EngineTorqueCurve;
	//Idle RPM
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float IdleRpm;
	//Max rpm 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Engine)
	float MaxRpm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Transmission)
	float TransmissionEfficiency;
	/*Affects rpm calculation ,tweak if rpm is not matching your expectations*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Transmission)
	float DifferentialRatio=1.0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Transmission)
	float GearChangeTime=0.5;
	UPROPERTY(EditDefaultsOnly,Category=Transmission)
    TArray<FArcadeGearInfo> Gears;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
    TEnumAsByte<ETraceTypeQuery> SuspensionTraceTypeQuery;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Suspension)
	float DampingCorrectionMultiplier=5;

	//steering Curve multiplies the steering value Time should be speed in KMH and Value should be Steering multiplier (ranges from 1-0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	FRuntimeFloatCurve SteerCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Steering)
	TEnumAsByte<EArcadeSteerType> SteerType;

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
};
