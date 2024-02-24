// Copyright Aurelion Iraj Mohtasham 2023. For distribution in Epic Store only.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "TankTireModel.generated.h"

/**
 * Represents a tank tire model.
 */
UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UTankTireModel : public UBaseTireModel
{
	// Tire model properties.
public:
	// Longitudinal friction multiplier for the tire.
	UPROPERTY(EditAnywhere, Category = TireModel)
	float FrictionMultiplierLongitudinal = 1.f;

	// Lateral friction multiplier for the tire.
	UPROPERTY(EditAnywhere, Category = TireModel)
	float FrictionMultiplierLateral = 1.f;

	// Reduce friction laterally when steering 
	UPROPERTY(EditAnywhere, Category = TireModel)
	float SteeringFrictionReductionMultiplier = 0.5f;
	
	// Radius of the sprocket for the tire.
	UPROPERTY(EditAnywhere, Category = TireModel)
	float SprocketRadius = 60.f;

	// Flag indicating whether brakes are used for steering.
	UPROPERTY(EditAnywhere, Category = TireModel)
	bool UseBrakesForSteering = true;

	// Normalized input for steering with brakes (0.0 to 1.0).
	UPROPERTY(EditAnywhere, Category = TireModel)
	float NormalizedSteeringBrakeInput = 0.5;

	GENERATED_BODY()

	// Function to update the tire simulation.
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;

	// Function to retrieve debug data for the tire.
	virtual FString GetTireDebugData(FVector2f& SlipData) override;

	// Normalized tire force in longitudinal and lateral directions.
	FVector2f TireForceNormalized;
};
