// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "BlueprintableTireModel.generated.h"

/**
 * 
 */
UCLASS(Blueprintable,EditInlineNew,Abstract)
class MODULARMOVEMENT_API UBlueprintableTireModel : public UBaseTireModel
{
	GENERATED_BODY()

	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;

	virtual void SetupWheels() override;

	virtual float GetTireStress() override;
public:
	/**
	 * 
	 * @param DeltaTime Time between this simulation step and previous one
	 * @param ModularMovementComponent Movement component for accessing data
	 * @param Wheel Wheel component for accessing data
	 * @param WheelSpaceVelocity X forward Y lateral relative to tire position (steer included)
	 * @param FinalForceVector Output force in newtons for simulation
	 * @param NewWheelAngularVelocity Output tire velocity in rad/s for animation
	 */
	UFUNCTION(BlueprintNativeEvent)
	void HandleSimulation(float DeltaTime, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel,FVector WheelSpaceVelocity, FVector& FinalForceVector,float& NewWheelAngularVelocity);

	UFUNCTION(BlueprintNativeEvent)
	void HandleInitializeSimulation();

	UFUNCTION(BlueprintNativeEvent)
	float HandleGetTireStress();
};
