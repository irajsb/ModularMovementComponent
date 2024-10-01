// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGearBox.h"
#include "SemiTruckGearBox.generated.h"

/**
 *  A gear box which will shift to ideal gear directly rather than based rpm range
 */
UCLASS()
class MODULARMOVEMENT_API USemiTruckGearBox : public UModularGearBox
{
	GENERATED_BODY()


	UPROPERTY(Category="Setup",EditAnywhere)
	float Cooldown=3.f;
	//Maximum RPM ratio difference between ideal rpm and current rpm before shift happens
	UPROPERTY(Category="Setup",EditAnywhere)
	float MaxVariance=0.3;
	float CurrentCooldown=0.f;

	virtual void Update(float DeltaTime, UModularMovementComponent* MovementComponent) override;
};
