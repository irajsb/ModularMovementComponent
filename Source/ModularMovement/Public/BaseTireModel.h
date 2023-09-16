//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Curves/CurveFloat.h"
#include "BaseTireModel.generated.h"


#define RELAXATION2(target, prev, rate) 			\
do {								\
double __tmp__;						\
__tmp__ = target;						\
target = (prev) + (rate) * ((target) - (prev)) * 0.01;	\
prev = __tmp__;						\
} while (0)


class UModularWheel;
class UModularMovementComponent;
/**
 * 
 */
UCLASS(DefaultToInstanced, Abstract)
class MODULARMOVEMENT_API UBaseTireModel : public UObject
{
	GENERATED_BODY()

public:
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
	                              UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
	{
		WheelOwner = Wheel;
	};

	virtual void SetupWheels()
	{
	};
	virtual float GetTireStress();

	UFUNCTION(BlueprintCallable,Category=Debug)
	virtual FString GetTireDebugData(FVector2f& SlipData)
	{
		return "";
	};
	UPROPERTY()
	UModularWheel* WheelOwner;
};
