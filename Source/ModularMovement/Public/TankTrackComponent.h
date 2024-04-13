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



UCLASS(BlueprintType,meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UTankTrackComponent : public USplineComponent
{
public:
	GENERATED_BODY()
	UTankTrackComponent();

	virtual void PostInitProperties() override;
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetupComponents(TArray<UTrackableComponent* > InWheels)	;

	void RebuildSplines(float DeltaTime, bool Editor);
	void UpdateMeshes(float DeltaTime, bool Editor);
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

	//Track offset to allign to sprocket teeth
	UPROPERTY(EditAnywhere, Category = "Tank Track",meta=(SliderExponent=0.0001))
	float ConstantOffset;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	UPROPERTY()
	UInstancedStaticMeshComponent * InstancedStaticMeshComponent;




	TArray<FTransform> UpdateInstanceTransforms;




	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float TrackSpeed;
	

	// Wheel radius / sprocket radius
	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float SprocketRatio;
	UPROPERTY(BlueprintReadOnly,Category="Tank Track")
	float SprocketRadius;
	float CurrentOffset;
	UPROPERTY()
	float OriginalSplineLen;
	UPROPERTY()
	float LenDiff;
	UPROPERTY()
	float OriginalTime;
	UPROPERTY()
	float SplinePointDiff;
	UPROPERTY()
	FVector OriginalSprocketContactPoint;
	
#if WITH_EDITOR

virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
