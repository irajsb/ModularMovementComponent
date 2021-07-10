// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ModularVehicleWheelData.h"
#include "WheelInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshWheel.generated.h"

/**
 * 
 */
UCLASS( meta=(BlueprintSpawnableComponent))
class TITANMOVEMENT_API USkeletalMeshWheel : public USkeletalMeshComponent,public IWheelInterface
{
	GENERATED_BODY()
	public:
	//Some Properties are not valid in SimulatedPawn
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FWheelState WheelState;
	virtual void SetupWheels(UModularMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateSuspension(float DeltaTime,UModularMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateForces(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent) override;
	virtual void UpdateSteering(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent, float SteeringAngle) override;
	virtual  void SetDriveTorqueOnWheels(float Force) override;
	virtual float GetFastestWheelOmegaSpeed() override;
	virtual int GetNumOfWheelsTouchingGround(bool OnlyDriveWheels) override;
	virtual void UpdateAnimation(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent) override;
	virtual void SimulateWheelData(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent) override;
	virtual FWheelState* GetWheelState() override;
	virtual FTransform GetWheelTransform() override;
	virtual void UpdateWheelState(FWheelState In) override;
};
