// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ModularVehicleWheelData.h"
#include "Components/SceneComponent.h"

#include "ModularWheel.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MODULARMOVEMENT_API UModularWheel : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UModularWheel();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	
	//Some Properties are not valid in SimulatedPawn
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FWheelState WheelState;
	virtual void SetupWheels(UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSuspension(float DeltaTime,UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent, float InNormSteering) ;
	virtual  void SetDriveTorqueOnWheels(float Force) ;
	virtual float GetFastestWheelOmegaSpeed() ;
	virtual int GetNumOfWheelsTouchingGround(bool OnlyDriveWheels) ;
	virtual void UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual FTransform GetWheelTransform() ;
	virtual void SimulateWheelData(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	 virtual void UpdateWheelState(FWheelState In) ;
	virtual FWheelState* GetWheelState() ;
	UModularVehicleWheelData* GetWheelSetup() const;
		
};
