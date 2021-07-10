// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ModularVehicleWheelData.h"
#include "Components/SceneComponent.h"
#include "WheelInterface.h"
#include "NoVisualWheel.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TITANMOVEMENT_API UNoVisualWheel : public USceneComponent,public IWheelInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNoVisualWheel();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
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
	virtual FTransform GetWheelTransform() override;
	virtual void SimulateWheelData(float DeltaTime, UModularMovementComponent* ArcadeMovementComponent) override;
	 virtual void UpdateWheelState(FWheelState In) override;
virtual FWheelState* GetWheelState() override;
		
};
