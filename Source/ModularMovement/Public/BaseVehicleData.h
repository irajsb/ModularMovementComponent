// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "BaseVehicleData.generated.h"

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

/**
 Base class for an engine model
 */
UCLASS(Abstract)
class MODULARMOVEMENT_API UBaseVehicleData : public UDataAsset
{
	GENERATED_BODY()



public:


	//all these functions will be inlined so there is no performance cost relative to directly accessing properties


	//engine
	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetIdleRPM()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetMaxRPM()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual bool ShouldZeroRpmWhenShifting()const{return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetTorqueForRPM(float RPM)const {return 0.f;};
	
	//Transmission

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual  TArray<FModularGearInfo> GetGears() const{return TArray<FModularGearInfo>();};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetGearChangeTime()const{return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetDifferentialRatio()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetTransmissionEfficiency()const {return 0.f;};

	

	//Misc
	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetAirDragConstant()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual bool ShouldScaleDriveTorqueToNumberOfWheels() const {return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	 virtual bool ShouldReverseAsBrake()const {return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetStopThreshold()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetWrongDirectionThreshold() {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual  TEnumAsByte<ETraceTypeQuery> GetSuspensionTraceTypeQuery()const{return  ETraceTypeQuery::TraceTypeQuery1;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetReverseThreshold()const{return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetIdleBrakeInput()const{return 0.f;};

	
	//steer

	 inline virtual float GetSteerSpeedScaleForSpeed(float Speed){return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual EModularSteerType GetSteerType()const{return EModularSteerType::SingleAngle;};

	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetSteerInputRise()const{return 0.f;};
	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetSteerInputFall()const{return 0.f;};
	UFUNCTION(BlueprintCallable,BlueprintPure)
	virtual float GetSteeringAnimationSpeed()const{return 0.f;};



	//NetWork
	UFUNCTION(BlueprintCallable,BlueprintPure)
	 virtual EVehicleNetworkMode GetNetworkMode()const{return EVehicleNetworkMode::ServerAuthoritative;}

	//AI
	virtual float GetAITraceLength()const{return 0.f;};
	virtual float GetAITraceSpeedMultiplier()const{return 0.f;};
	virtual float GetNearGoalDistance(){return 0.f;};
	virtual float GetDesireSpeedNearGoal()const{return 0.f;};
	virtual float GetDesireSpeedNormal()const{return 0.f;};
	virtual float GetTurnThreshold()const{return 0.f;};
	virtual float GetDesireSpeedTurning()const{return 0.f;};
	virtual float GetDesireSpeedTurningAround()const{return 0.f;};
	virtual float GetFullThrottleSpeed()const{return 0.f;};
	virtual float GetAIMaxSteerMultiplier()const{return 0.f;};
};
