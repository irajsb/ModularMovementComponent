//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TrackableComponent.generated.h"

/*
 * A component that can be used to build a Tank track
 */


USTRUCT()
struct FCustomTrackTangent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,Category=Track)
	int ContactPointIndex=0;

	UPROPERTY(EditAnywhere,Category=Track)
	FVector Tangent=FVector::ZeroVector;

	UPROPERTY(EditAnywhere,Category=Track)
	float Angle=0.f;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MODULARMOVEMENT_API UTrackableComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTrackableComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



	//Contact points relative to radius. for example a contact point of (0,1) is top of the wheel and (1,0) is right side of the wheel and (0,-1) is down side of the wheel)
	UPROPERTY(EditAnywhere,Category="Track")
	TArray<FVector2D>ContactPoints;

	//Contact points relative to radius. for example a contact point of (0,1) is top of the wheel and (1,0) is right side of the wheel and (0,-1) is down side of the wheel)
	UPROPERTY(EditAnywhere,Category="Track")
	TArray<FCustomTrackTangent>CustomTangentArray;
	
	
	
};
