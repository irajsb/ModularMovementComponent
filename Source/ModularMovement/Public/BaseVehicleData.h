//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"


#include "Engine/DataAsset.h"
#include "BaseVehicleData.generated.h"

UENUM(BlueprintType)
enum  EModularSteerType 
{
	SingleAngle,
	Ackermann,
	Tank,
};

UENUM(BlueprintType)
enum EVehicleNetworkMode
{	ClientPredictive,
	ServerAuthoritative,
	ClientAuthoritative,
	 
};


/**
 Base class for an engine model
 */
UCLASS(Abstract,Blueprintable)
class MODULARMOVEMENT_API UBaseVehicleData : public UObject
{
	GENERATED_BODY()



public:


	//all these functions will be inlined so there is no performance cost relative to directly accessing properties


	//engine
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetIdleRPM()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetMaxRPM()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual bool ShouldZeroRpmWhenShifting()const{return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetTorqueForRPM(float RPM)const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetEngineInertia()const {return 0.f;};
	//Transmission

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual class UModularGearBox* GetGearBox(){return nullptr;}



	

	//Misc
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetAirDragConstant()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual bool ShouldScaleDriveTorqueToNumberOfWheels() const {return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	 virtual bool ShouldReverseAsBrake()const {return false;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetStopThreshold()const {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetWrongDirectionThreshold() {return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual  TEnumAsByte<ETraceTypeQuery> GetSuspensionTraceTypeQuery()const{return  ETraceTypeQuery::TraceTypeQuery1;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetReverseThreshold()const{return 0.f;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetIdleBrakeInput()const{return 0.f;};

	
	//steer

	 inline virtual float GetSteerSpeedScaleForSpeed(float Speed){return 0.f;};
	
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual EModularSteerType GetSteerType()const{return EModularSteerType::SingleAngle;};

	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetSteerInputRise()const{return 0.f;};
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetSteerInputFall()const{return 0.f;};
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual  void GetAckermannValues(float & WheelBase,float & TrackWidth){}
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual float GetCounterSteerMultiplier()const{return 3.f;};
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
	virtual bool GetScaleTireFrictionWithSurfaceAngle(){return true;}

	//NetWork
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Setup)
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
