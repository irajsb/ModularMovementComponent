// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BaseModularMovementComponent.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class MODULARMOVEMENT_API UBaseModularMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()


public:

	//Input Forwarding Specific
	UPROPERTY(Transient)
	bool bShouldReplicateInput=false;


	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void HoldStarter(float StartTime){};

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void ReleaseStarter(){};

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void StopEngine(){};



	// Set input functions
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void SetThrottleInput(float Input){};

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void SetSteeringInput(float Input){};

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void SetBrakeInput(float Brake){};

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	virtual void SetHandBrakeInput(bool Brake){};
	
};
