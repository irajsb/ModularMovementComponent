// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "ArcadeMovementComponent.h"
#include "ArcadeWheelInterface.h"

#include "SimpleStaticMeshWheel.generated.h"

/**
 * 
 */


UCLASS( meta=(BlueprintSpawnableComponent))
class TITANMOVEMENT_API USimpleStaticMeshWheel : public UStaticMeshComponent,public IArcadeWheelInterface
{
	
	public:
	USimpleStaticMeshWheel();
	GENERATED_BODY()

	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FWheelState WheelState;
	virtual void SetupWheels(UArcadeMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateSuspension(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateForces(float DeltaTime, UArcadeMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateSteering(float DeltaTime, UArcadeMovementComponent* ArcadeMovementComponent, float SteeringAngle) override;
	virtual  void SetDriveTorqueOnWheels(float Force) override;
	virtual float GetFastestWheelOmegaSpeed() override;
};
