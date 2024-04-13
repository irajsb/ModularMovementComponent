// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "PropellerModel.generated.h"

/**
 * 
 */


// Simple air/ sea propeller

UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UPropellerModel : public UBaseTireModel
{
	GENERATED_BODY()


public:

	// moment of inertia (in kg*m^2)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float MomentOfInertia = 10;
	//  thrust coefficient
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float Ct = 0.5;
	//  density of water (in kg/m^3)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float Rho = 1000;

	//  area swept by propeller blades (in m^2)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float A = 1; 


	//  drag coefficient
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float DragCoefficient = 0.1;
	//  area of resistance (in m^2)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	 float AreaOfResistance = 0.5;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Propeller)
	float PropellerRadius=1.f;
	
	static double CalculateThrust(double Ct, double rho, double A, double omega);
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;

	virtual FString GetTireDebugData(FVector2f& SlipData) override;
	FVector TireForceNormalized;
};


