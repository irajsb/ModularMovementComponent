// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ArcadeWheelInterface.generated.h"

/**
 * 
 */
class UArcadeMovementComponent;
UINTERFACE()
class TITANMOVEMENT_API UArcadeWheelInterface : public UInterface
{
	GENERATED_BODY()
	
};

class IArcadeWheelInterface
{

	GENERATED_BODY()

public:
	virtual void UpdateSuspension(float DeltaTime,UArcadeMovementComponent* ArcadeMovementComponent){};
	
};