// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ModularVehicleWheelData.h"
#include "Components/SceneComponent.h"

#include "ModularWheel.generated.h"

class UVehicleDebugWidget;
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
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ShowOnlyInnerProperties ))
	FWheelState WheelState;
	virtual void SetupWheels(UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSuspension(float DeltaTime,UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent, float InNormSteering) ;
	virtual  void SetDriveTorqueOnWheels(float Force) ;
	virtual float GetFastestWheelOmegaSpeed() ;
	virtual void UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual FTransform GetWheelTransform() ;

	 virtual void UpdateWheelState(FWheelState In) ;
	virtual FWheelState* GetWheelState() ;
	UModularVehicleWheelData* GetWheelSetup() const;


	///BP

	//Change Wheel Setup
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData);
	//Update Wheel Steering 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void SetSteerOnWheel(float Angle );



	//Debug

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 void AddDebugWidgetToWheel(TSubclassOf<UVehicleDebugWidget> Widget);
};
