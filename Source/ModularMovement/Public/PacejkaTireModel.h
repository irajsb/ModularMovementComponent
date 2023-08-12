// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "PacejkaTireModel.generated.h"

/**
 * 
 */

#define RELAXATION2(target, prev, rate) 			\
do {								\
double __tmp__;						\
__tmp__ = target;						\
target = (prev) + (rate) * ((target) - (prev)) * 0.01;	\
prev = __tmp__;						\
} while (0)

UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UPacejkaTireModel : public UBaseTireModel 
{
	GENERATED_BODY()

	//
	UPROPERTY(EditAnywhere)
	float MU=1;
	UPROPERTY(EditAnywhere)
	float LoadFactorMin=0.8;
	UPROPERTY(EditAnywhere)
	float LoadFactorMax=1.6;
	UPROPERTY(EditAnywhere)
	float PacejkaBLong=10.f;
	UPROPERTY(EditAnywhere)
	float PacejkaCLong=1.9f;
	UPROPERTY(EditAnywhere)
	float PacejkaDLong=1.f;
	UPROPERTY(EditAnywhere)
	float PacejkaELong=0.97;
	UPROPERTY(EditAnywhere)
	float PacejkaBLat=15.2;
	UPROPERTY(EditAnywhere)
	float PacejkaCLat=1.6;
	UPROPERTY(EditAnywhere)
	float PacejkaDLat=1.f;
	UPROPERTY(EditAnywhere)
	float PacejkaELat=-1.6;
	UPROPERTY(EditAnywhere)
	float TireFrictionCoefficient=1.f;
	
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;
	virtual float GetTireStress() override;

	FVector2f TireForceNormalized;
	float TireStress;
	virtual FString GetTireDebugData(FVector2f& SlipData) override;

	float LastFn,LastFt,LastSlipX;
	
};
