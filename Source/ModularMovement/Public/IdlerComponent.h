//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "TrackableComponent.h"
#include "IdlerComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable,meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UIdlerComponent : public UTrackableComponent
{
	GENERATED_BODY()
public:

	UIdlerComponent();
	//Radius where track will be offseted from center 
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category=Track)
	float Radius;

	
	UPROPERTY()
	FVector OriginalLocation;

	virtual void BeginPlay() override;
};

