// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "TankTireModel.generated.h"

/**
 * 
 */
UCLASS()
class MODULARMOVEMENT_API UTankTireModel : public UBaseTireModel
{

	UPROPERTY(EditAnywhere)
	float FrictionMultiplierLongitudinal=1.f;
	UPROPERTY(EditAnywhere)
	float FrictionMultiplierLateral=1.f;
	UPROPERTY(EditAnywhere)
	float SprocketRadius=60.f;
	UPROPERTY(EditAnywhere)
	bool UseBrakesForSteering=true;
	UPROPERTY(EditAnywhere)
	float NormalizedSteeringBrakeInput=0.5;
	GENERATED_BODY()
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;
	
};
