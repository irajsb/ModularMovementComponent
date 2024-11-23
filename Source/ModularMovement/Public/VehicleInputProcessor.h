// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleInputProcessor.generated.h"

class UModularMovementComponent;
/**
 Class for manipulating input based on vehicle state
 */
UCLASS(Blueprintable,BlueprintType)
class MODULARMOVEMENT_API UVehicleInputProcessor : public UObject
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent,Category="Input")
	float CalcBrakeInput(UModularMovementComponent* MovementComponent,float DeltaTime,float RawBrakeInput,float RawThrottleInput);

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent,Category="Input")
	float CalcSteerInput(UModularMovementComponent* MovementComponent,float DeltaTime,float RawInput);

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent,Category="Input")
	float CalcThrottleInput(UModularMovementComponent* MovementComponent,float DeltaTime,float RawInput,float RawBrakeInput,float RawSteeringInput);


	static float CalculateOptimalDriftAngle(const UModularMovementComponent* ModularMovement,float Velocity);

	static float CalculateSteeringCorrection(float AngleError, const UModularMovementComponent* ModularMovement,float InSteer) ;
	static float CalculateThrottleAdjustment(const UModularMovementComponent* ModularMovement, float InThrottle);

	
	// Main drift assistance functions
	UFUNCTION(BlueprintCallable, Category = "Drift Assist")
	static bool IsDrifting(const UModularMovementComponent* ModularMovement);
};
