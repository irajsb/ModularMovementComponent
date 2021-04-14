// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ArcadeWheelInterface.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ArcadeMovementComponent.generated.h"
class AArcadePawn;




struct FArcadeVehicleDebugParams
{
	bool ShowSuspensionDebug=false;
};


USTRUCT(BlueprintType)
struct FArcadeWheelSetup
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
	//amount of friction when moving forward
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float LongitudinalFrictionMultiplier=1.0;
	//amount of friction when moving Side ways
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float LateralFrictionMultiplier=1.0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool ABSEnabled;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool TractionControlEnabled;
	

	
};
USTRUCT(BlueprintType)
struct FWheelState{
	GENERATED_BODY()
	//Ranges from 0-1
	float PreviousLen;
	float SteerAngle=0;
	UPROPERTY()
	FHitResult HitResult;
	bool bIsSlipping;
	//SuspensionForce That was applied;
	FVector WheelLoad;
	FVector PreviousWheelCollisionVelocity;
	UPROPERTY(EditAnywhere)
	FArcadeWheelSetup WheelSetup;
	float DriveTorque;
	float BrakeTorque;
	float Spin;
	bool Spinning;
	float Omega;	// [radians/sec] Wheel Rotation Angular Velocity
	float AngularPosition;			// [radians]
	
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

	FVector InputVector;
	UPROPERTY(BlueprintReadOnly)
	TArray<UActorComponent*> Components;
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
	
	UFUNCTION(BlueprintCallable)
	int GetNumberOfWheels();
	FArcadeGearInfo GetGearInfo(int Index);
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void UpdateEngine(float DeltaTime);
	void UpdateSuspension(float DeltaTime);
	void UpdateForces(float DeltaTime);
	//trace and apply forces
	  void WheelTrace(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	  void ApplyWheelForces(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	
	float CmToM(float In);

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

	
	
};

