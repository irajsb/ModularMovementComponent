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
	UPROPERTY(EditAnywhere)
	bool AnimateChildComponent;
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
	UFUNCTION(BlueprintCallable)
	UModularVehicleWheelData* GetWheelSetup() const;

	
	///BP

	//Change Wheel Setup
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData);
	//Update Wheel Steering 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void SetSteerOnWheel(float Angle );
	//actual rotation does not give you acuurate steering and rotation values its has some corrections applied to it so get steering from dedicated values (additional data returned don't affect performance )
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 void GetWheelAnimationData(FVector& Location,FRotator& Rotation,float DeltaTime);
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelRotation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelPivotRotation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelSteeringValue();
	//1 fully compressed 0 fully extended 
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelCompressionValue();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelRPM();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	bool IsWheelTouchingGround();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	FVector GetWheelCenterLocation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	float GetDampingForce();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	float GetTireStress();

	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	UBaseTireModel* GetTireModel();

	//Debug

	UFUNCTION(BlueprintCallable)
	void ChangeTraceDebugVisbility(bool Enable);
	private:
	bool Debug=false;



	Chaos::FRigidBodyHandle_Internal* GetInternalHandle(UPrimitiveComponent* Component, FName BoneName);
	void AddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName);
	
};
