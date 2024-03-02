//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "VehicleAudioComponent.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UVehicleAudioComponent : public UAudioComponent
{
	GENERATED_BODY()
	UVehicleAudioComponent();
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float RPMInterpolationSpeed=0.5;
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float RPMMultiplier=1;
	
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float LoadInterpolationSpeed=1;
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float LoadMultiplier=1;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float RPM=0.f;
	float Load;
};
