// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseTireModel.generated.h"

class UModularWheel;
class UModularMovementComponent;
/**
 * 
 */
UCLASS(EditInlineNew,Abstract)
class MODULARMOVEMENT_API UBaseTireModel : public UObject
{
	GENERATED_BODY()

public:
	virtual void UpdateSimulation(float DeltaTime,FVector& FinalForceVector,UModularMovementComponent* ModularMovementComponent,UModularWheel* Wheel)
	{
		WheelOwner=Wheel;
	};
	virtual float GetTireStress();

	UFUNCTION(BlueprintCallable)
	virtual FString GetTireDebugData(FVector2f& SlipData)
	{
		return "";
	};
	UPROPERTY()
	UModularWheel* WheelOwner;
};
