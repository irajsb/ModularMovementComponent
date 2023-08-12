// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "ModularWheel.h"
#include "ArcadeTireModel.generated.h"

class UModularMovementComponent;
/**
 * 
 */
UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UArcadeTireModel : public UBaseTireModel
{
	GENERATED_BODY()
	UArcadeTireModel();
	UPROPERTY(EditAnywhere)
	float FrictionMultiplierLongitudinal;
	
	
	UPROPERTY(EditAnywhere)
	bool AutoGenerateTheGraph;
	UPROPERTY(EditAnywhere,meta=(EditCondition=AutoGenerateTheGraph))
	float MinFrictionLateralForce=0.8;
	UPROPERTY(EditAnywhere,meta=(EditCondition=AutoGenerateTheGraph))
	float MaxFrictionLateralForce=1.1;

	UPROPERTY(EditAnywhere)
	FRuntimeFloatCurve LateralGripCurve;
	void RebuildCurves(bool Force);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void UpdateSimulation(float DeltaTime,FVector& FinalForceVector,UModularMovementComponent* ModularMovementComponent,UModularWheel* Wheel) override;
	
};

