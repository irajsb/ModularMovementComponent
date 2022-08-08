// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "VehicleSpringArm.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=Camera, meta=(BlueprintSpawnableComponent), hideCategories=(Mobility))
class MODULARMOVEMENT_API UVehicleSpringArm : public USpringArmComponent
{
	GENERATED_BODY()
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	public:
	//Minimum speed to start interpolating CM/s
	UPROPERTY(EditAnywhere)
	float MinSpeedRange=5;
	//Max speed to start interpolating CM/s
	UPROPERTY(EditAnywhere)
	float MaxSpeedRange=500;
	//Interplolation Scale 
	UPROPERTY(EditAnywhere)
	float InterpolationStrength=120;
	UPROPERTY(EditAnywhere)
	bool IgnorePitch=true;

	UPROPERTY(EditAnywhere)
	float MinArmLen=450;
	UPROPERTY(EditAnywhere)
	float MaxArmLen=550;
	UPROPERTY(EditAnywhere)
	float MaxAccel=10;
	UPROPERTY(EditAnywhere)
	float MinAccel=-10;
	UPROPERTY(EditAnywhere)
	float ArmLenAnimSpeed=100;
	//Adds a cooldown in seconds  for when we need to not auto update camera to velocity 
	UFUNCTION(BlueprintCallable)
	void SetCooldown(float In);

	private:
	float CoolDown;
	FVector PreviousVelocity;
};
