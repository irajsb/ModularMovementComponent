//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Curves/CurveFloat.h"
#include  "PhysicalMaterials/PhysicalMaterial.h"

#include "DrawDebugHelpers.h"

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
	//Use constant wheel load instead of real one. makes vehicle more arcade style and easier to control
	UPROPERTY(EditAnywhere,Category=AdvancedTire)
	bool UseConstantWheelLoad=true;


	/**
	Friction limit is combined
	For example when burning out or handbraking tire reaches it limit both laterally and longitudinally,
	allowing it to slide sideways.
	Disabling this allows for easier manoeuvrability in corners and more arcade feel  )
	 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=AdvancedTire)
	bool UseCombinedFriction=true;
	//Enable UseCombinedFriction When handbraking
	UPROPERTY(EditAnywhere,Category=AdvancedTire)
	bool UseCombinedFrictionWhenHandBraking=true;
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

	
	UFUNCTION(BlueprintCallable,Category=Data)
	virtual void RefreshTireData();
};
