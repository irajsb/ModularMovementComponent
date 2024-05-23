//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "BaseTireModel.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "PacejkaTireModel.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FPacejkaConstants
{
	GENERATED_BODY()

	//Stiffness
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	float B=20.f;
	//Shape
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	float C=1.4f;
	//Curvature
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	float E=-0.05f;
	//Peak Force
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	float D=1.2f;

	FPacejkaConstants(float InB,float InC,float InD,float InE)
	{
		B=InB;
		C=InC;
		D=InD;
		E=InE;
	}
	FPacejkaConstants(){}

};


UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UPacejkaTireModel : public UBaseTireModel 
{
	GENERATED_BODY()

public:

	UPacejkaTireModel();
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	FPacejkaConstants Long;
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category=TireModel)
	FPacejkaConstants Lat=FPacejkaConstants(10.f,1.3f,1.2f,-0.2);
	

	
	virtual void UpdateSimulation(float DeltaTime, FVector& FinalForceVector, UPrimitiveComponent* Mesh, UModularMovementComponent*
	                              ModularMovementComponent, UModularWheel* Wheel) override;
	virtual float GetTireStress() override;
	virtual void SetupWheels() override;
	FVector2f TireForceNormalized;

	virtual FString GetTireDebugData(FVector2f& SlipData) override;

	float LastFX,LastFY,Speak,SideSlipPeak ;
	
};
