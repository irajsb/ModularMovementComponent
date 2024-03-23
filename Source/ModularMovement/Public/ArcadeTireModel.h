//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"

#include "BaseTireModel.h"
#include "ModularWheel.h"
#include "Curves/CurveFloat.h"
#include "ArcadeTireModel.generated.h"

class UModularMovementComponent;
/**
 * 
 */
UCLASS(EditInlineNew,HideCategories=AdvancedTire)
class MODULARMOVEMENT_API UArcadeTireModel : public UBaseTireModel
{
	GENERATED_BODY()
	UArcadeTireModel();
	//Friction multiplier on forward axis of the wheel
	UPROPERTY(EditAnywhere,Category=TireModel)
	float FrictionMultiplierLongitudinal = 1.f;

	//Will generate a general graph based on values below
	UPROPERTY(EditAnywhere, Category=TireModel)
	bool AutoGenerateTheGraph;
	// Friction when traction is lost in lateral
	UPROPERTY(EditAnywhere, Category=TireModel, meta=(EditCondition=AutoGenerateTheGraph))
	float MinFrictionLateralForce = 0.8;
	//Friction when wheel has perfect grip in lateral 
	UPROPERTY(EditAnywhere, Category=TireModel, meta=(EditCondition=AutoGenerateTheGraph))
	float MaxFrictionLateralForce = 1.1;
	//Lateral force curve 
	UPROPERTY(EditAnywhere, Category=TireModel)
	FRuntimeFloatCurve LateralGripCurve;
	//This value will combine the frictions so they can affect each other . for example when wheels are locked by hand brake we want vehicle to also have less friction in lateral
	UPROPERTY(EditAnywhere, Category=TireModel)
	FVector2D FrictionEllipse = FVector2D(1.f, 1.f);
	void RebuildCurves(bool Force);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
	                              UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel) override;
	virtual FString GetTireDebugData(FVector2f& SlipData) override;

	FVector2f TireForceNormalized;
};
