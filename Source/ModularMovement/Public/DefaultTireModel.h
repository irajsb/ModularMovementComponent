// Copyright Aurelion Iraj Mohtasham 2023. For distribution in Epic Store only.

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "DefaultTireModel.generated.h"

/**
 * Can produce realistic and arcade results based on input given to it.
 * This is our default tire model.
 * Easy to tune.
 */
UCLASS(EditInlineNew,BlueprintType)
class MODULARMOVEMENT_API UDefaultTireModel : public UBaseTireModel
{
	GENERATED_BODY()

	UDefaultTireModel();

public:
	// Auto generate graph using the min and max parameters.
	UPROPERTY(EditAnywhere, Category = TireModel)
	bool AutoGenerateGraph;

	// Minimum friction force for auto-generated graph.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel, meta = (EditCondition = AutoGenerateGraph))
	float MinFrictionForce = 0.8;

	// Maximum friction force for auto-generated graph.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel, meta = (EditCondition = AutoGenerateGraph))
	float MaxFrictionForce = 1.5;

	// Longitudinal grip curve for the tire.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel)
	FRuntimeFloatCurve LongitudinalGripCurve;

	// Auto generate the lateral graph using the min and max parameters.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel)
	bool AutoGenerateTheLateralGraph;

	// Minimum lateral friction force for auto-generated graph.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel, meta = (EditCondition = AutoGenerateGraph))
	float MinFrictionLateralForce = 0.8;

	// Maximum lateral friction force for auto-generated graph.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel, meta = (EditCondition = AutoGenerateGraph))
	float MaxFrictionLateralForce = 1.5;

	// Lateral grip curve for the tire.
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = TireModel)
	FRuntimeFloatCurve LateralGripCurve;

	float LastSlipX;
	FVector2f TireForceNormalized;
	float LastFX, LastFY, Speak, SideSlipPeak;

	// Rebuild the curves for longitudinal and lateral grip.
	void RebuildCurves(bool ForceLong, bool ForceLateral);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;
	virtual FString GetTireDebugData(FVector2f& SlipData) override;
	virtual void SetupWheels() override;

	virtual void RefreshTireData() override;
};
