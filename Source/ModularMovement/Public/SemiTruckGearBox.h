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


	UPROPERTY(EditAnywhere)
	float Cooldown=3.f;
	
	float CurrentCooldown;

	virtual void Update(float DeltaTime, UModularMovementComponent* MovementComponent) override;
};
