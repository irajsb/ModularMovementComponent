//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "ModularWheel.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TankTrackComponent.generated.h"

/**
 * A component that builds an spline around the wheels and then spawns instanced meshes as each track and moves them around it 
 */


USTRUCT()
struct MODULARMOVEMENT_API FTrackSharedData
{
	GENERATED_BODY()

	FTrackSharedData() = default;

	FTrackSharedData(UInstancedStaticMeshComponent* InISMC)
		: ISMC(InISMC),
		RefCount(1)
	{

	}

	UInstancedStaticMeshComponent* ISMC = nullptr;

	int32 RefCount = 1;

	/** Buffer holding current frame transforms for the static mesh instances, used to batch update the transforms */
	TArray<int32> UpdateInstanceIds;
	TArray<FTransform> StaticMeshInstanceTransforms;
	TArray<FTransform> StaticMeshInstancePrevTransforms;

	/** Buffer holding current frame custom floats for the static mesh instances, used to batch update the ISMs custom data */
	TArray<float> StaticMeshInstanceCustomFloats;

	// When initially adding to StaticMeshInstanceCustomFloats, can use the size as the write iterator, but on subsequent processors, we need to know where to start writing
	int32 WriteIterator = 0;
};


UCLASS(BlueprintType,meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UTankTrackComponent : public USplineComponent
{
public:
	GENERATED_BODY()
	UTankTrackComponent();

	virtual void PostInitProperties() override;
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetupComponents(TArray<UTrackableComponent* > InWheels)	;

	void RebuildSplines(float DeltaTime);
	void UpdateMeshes(float DeltaTime);
	UPROPERTY()
	TArray<UTrackableComponent* > Wheels;


	UPROPERTY(EditAnywhere, Category = "Tank Track")
	UStaticMesh* TrackMesh;

	

	UPROPERTY(EditAnywhere, Category = "Tank Track")
	FVector Scale = FVector(1, 1, 1);

	UPROPERTY(EditAnywhere, Category = "Tank Track")
	int NumOfMeshesInTrack = 40;

	// Offset track location from the wheel
	UPROPERTY(EditAnywhere, Category = "Tank Track")
	FVector WheelOffset;

	UPROPERTY(EditAnywhere, Category = "Tank Track")
	FVector IdlerOffset;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	UPROPERTY()
	UInstancedStaticMeshComponent * InstancedStaticMeshComponent;

	TArray<FTrackSharedData> ISMCSharedData;
	
	TArray<int32> UpdateInstanceIds;
	TArray<FTransform> UpdateInstanceTransforms;
	TArray<FTransform> UpdateInstancePreviousTransforms;
	
	int32 InNumCustomDataFloats;
	TArray<float> CustomFloatData;
	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float TrackSpeed;
	

	// Wheel radius / sprocket radius
	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float SprocketRatio;
	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float SprocketRadius;
	float CurrentOffset;
	
	
};
