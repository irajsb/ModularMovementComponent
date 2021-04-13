// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ArcadeWheelInterface.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ArcadeMovementComponent.generated.h"
class AArcadePawn;


/**
 * 
 */
//Why separate struct for data and state? cause if we needed to replicate data does not need to be replicated but maybe state needs to

struct FArcadeVehicleDebugParams
{
	bool ShowSuspensionDebug=false;
};
USTRUCT(BlueprintType)
struct FWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	float PreviousLen;
	
};
USTRUCT(BlueprintType)
struct FArcadeWheelInfo
{	GENERATED_BODY()
	//How much can wheels drop
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float SuspensionLength=50;
	//trace wheel radius
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float WheelRadius=30;

	//Offset to apply to trace start
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVector TraceStartOffset;
	//Force to apply
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float Stiffness;
	FWheelState WheelState;

	
};

USTRUCT(BlueprintType)
struct FEngineData
{
	GENERATED_BODY()
	//Torque curve 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRuntimeFloatCurve EngineTorqueCurve;
	//Idle RPM
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinRpm;
	//Max rpm 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxRpm;
};

USTRUCT(BlueprintType)
struct FArcadeGearInfo
{
	GENERATED_USTRUCT_BODY()
	//This Gear's Ratio
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float GearRatio;
	/** Value of engineRevs/maxEngineRevs that is low enough to gear down */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DownRatio;

	/** Value of engineRevs/maxEngineRevs that is high enough to gear up */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float UpRatio;
	FArcadeGearInfo():GearRatio(1.0f),DownRatio(0.1),UpRatio(1.f)
	{
		
	};
	FArcadeGearInfo(float Ratio):GearRatio(Ratio),DownRatio(0.1),UpRatio(1.f)
	{
	}
};
UCLASS()
class TITANMOVEMENT_API UArcadeMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()


	UArcadeMovementComponent();
	public:
	UMeshComponent* GetMesh();
	//Engine
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category= Setup)
	FEngineData EngineConfig;
	

	UPROPERTY(BlueprintReadOnly)
	float CurrentRpm;
	UPROPERTY(BlueprintReadOnly)
	float CurrentRpmRatio;
	//eof engine
	// Idle gear should be 0 ,gears before 0 are back after 0 are forward 
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TArray<FArcadeGearInfo> Gears;
	/*Affects rpm calculation ,tweak if rpm is not matching your expectations*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float DifferentialRatio=1.0;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float DampingCorrectionMultiplier=0.5;
	UPROPERTY(BlueprintReadOnly)
	int IdleGear;
	UPROPERTY(BlueprintReadOnly)
	int CurrentGear;


	//susp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	TEnumAsByte<ETraceTypeQuery> SuspensionTraceTypeQuery;
	

	FArcadeGearInfo GetGearInfo(int Index);
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void UpdateEngine(float DeltaTime);
	void UpdateSuspension(float DeltaTime);
	//trace and applyforces
	  void WheelTrace(UWorld* World,FArcadeWheelInfo& WheelInfo,float DeltaTime,USceneComponent* ArcadeWheel);

	


	//debug

	void ShowDebugInfo(class AHUD* HUD, class UCanvas* Canvas, const class FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);
	/** Draw 2D debug text graphs on UI for the wheels, suspension and other systems */
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	float CalcDialAngle(float CurrentValue, float MaxValue);
	void DrawDial(UCanvas* Canvas, FVector2D Pos, float Radius, float CurrentValue, float MaxValue);
	// draw 2D debug line to UI canvas
	void DrawLine2D(UCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, FColor Color, float Thickness = 1.f);

	
#endif
	UPROPERTY(EditAnywhere)
	bool bDebugMode;
	private:
	
};

