// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ArcadeWheelInterface.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ModularVehicleData.h"
#include"ModularVehicleWheelData.h"
#include "ArcadeMovementComponent.generated.h"
class AArcadePawn;




struct FArcadeVehicleDebugParams
{
	bool ShowSuspensionDebug=false;
	bool ShowInputProcessingDebug=false;
	bool ShowGearboxLog=false;
	bool ShowDrawFriction=false;
};




USTRUCT(BlueprintType)
struct FVehicleState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category= Setup)
	UModularVehicleData* VehicleData;

	UPROPERTY(BlueprintReadWrite)
	float CurrentRpm;
	UPROPERTY(BlueprintReadWrite)
	float CurrentRpmRatio;
	//eof engine
	// Idle gear should be 0 ,gears before 0 are back after 0 are forward 
	UPROPERTY(BlueprintReadWrite)
	int IdleGear;
	UPROPERTY(BlueprintReadWrite)
	int CurrentGear;
	UPROPERTY(BlueprintReadWrite)
	int TargetGear;
	UPROPERTY(BlueprintReadWrite)
	float ForwardSpeed;
	UPROPERTY(BlueprintReadWrite)
	float CurrentGearChangeTime;

};



UCLASS()
class TITANMOVEMENT_API UArcadeMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

	//Constructor
	UArcadeMovementComponent();
	public:
	//Wheels
	UPROPERTY(BlueprintReadOnly)
	TArray<UActorComponent*> Components;
	//Return Mesh
	UMeshComponent* GetMesh();
	private:
	
	UPROPERTY(Transient)
	float RawBrakeInput;
	// What the player has the steering set to. Range -1...1
	UPROPERTY(Transient)
	float RawSteeringInput;

	// What the player has the accelerator set to. Range -1...1
	UPROPERTY(Transient)
	float RawThrottleInput;
	UPROPERTY(Transient)
	bool bRawHandbrakeInput;
public:
	//input 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetInputThrottle(float Input);
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetInputSteering(float Input);
	/** Set the user input for the vehicle Brake [range 0 to 1] set it if you've turned of reverse as brake */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetBrakeInput(float Brake);
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FVehicleState VehicleState;

	/** Compute steering input */
	float CalcSteeringInput();

	/** Compute brake input */
	float CalcBrakeInput();

	/** Compute handbrake input */
	float CalcHandbrakeInput();

	float CalcThrottleInput();
	//Engine

	void SetTargetGear(int32 GearNum, bool bImmediate);
	//Get Number of wheels(some components are allowed to have more than one wheel thats why we just dont count components
	UFUNCTION(BlueprintCallable)
	int GetNumberOfWheels();
	//
	FArcadeGearInfo GetGearInfo(int Index);
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void UpdateState(float DeltaTime);
	void UpdateGearBox(float DeltaTime);
	void UpdateEngine(float DeltaTime);
	void UpdateSuspension(float DeltaTime);
	void UpdateForces(float DeltaTime);
	void UpdateSteering(float DeltaTime);
	//trace and apply Suspension forces
	void WheelTrace(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	//Apply Drive Brake and friction
	void ApplyWheelForces(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	void CalculateSteeringAngle(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel,float InNormSteering);

	
	//ChaosDefault
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

