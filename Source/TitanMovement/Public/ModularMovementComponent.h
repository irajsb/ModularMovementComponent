// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"

#include "WheelInterface.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ModularVehicleData.h"
#include"ModularVehicleWheelData.h"
#include "ModularMovementComponent.generated.h"
class AArcadePawn;


//Cosmetic delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGearChange,int,CurrentGear,int,TargetGear,bool,Finished);

struct FModularVehicleDebugParams
{
	int ShowSuspensionDebug=false;
	int ShowInputProcessingDebug=false;
	int ShowGearboxLog=false;
	int ShowDrawFriction=false;
	int AIDebug=false;
};


UENUM()
enum EAIVehicleState {/*target is in front*/Normal,/*target is in back*/TurningAround  };

USTRUCT()
struct FRepCosmeticData
{
	GENERATED_USTRUCT_BODY()

    /** Engine RPM */
    UPROPERTY()
	uint8 EngineRPM;

	uint8 CurrentGear;

	

	FRepCosmeticData()
	{
		EngineRPM = 0;
		CurrentGear=0;
	}
};

USTRUCT()
struct FModularTrackInfo
{
	GENERATED_BODY()

	float TorqueTransfer;

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

	float DesiredSpeed;
	float AIPreviousThrottle;
	EAIVehicleState AIState;
	bool IsAIVehicle;
	float LockCurrentStateDelta;
	int DriveWheelsOnGround;
	FModularTrackInfo TrackLeft;
	FModularTrackInfo TrackRight;
	
	
};



UCLASS(meta=(BlueprintSpawnableComponent))
class TITANMOVEMENT_API UModularMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

	//Constructor
	UModularMovementComponent();
	void UpdateComponents();
	public:
	//Wheels
	UPROPERTY()
	TArray<UActorComponent*> Components;
	//Return Mesh
	UMeshComponent* GetMesh()const;
	private:
	
	UPROPERTY(Transient)
	float RawBrakeInput;
	// What the player has the steering set to. Range -1...1
	UPROPERTY(Transient)
	float RawSteeringInput;
	UPROPERTY(Transient)
	float SteeringInput;
	UPROPERTY(Transient)
	float BrakeInput;

	// What the player has the accelerator set to. Range -1...1
	UPROPERTY(Transient)
	float RawThrottleInput;
	UPROPERTY(Transient)
	bool HandBrakeInput;
public:
	//input 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetThrottleInput(float Input);
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetSteeringInput(float Input);
	/** Set the user input for the vehicle Brake [range 0 to 1] set it if you've turned off reverse as brake */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetBrakeInput(float Brake);

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetHandBrakeInput(bool Brake);
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FVehicleState VehicleState;

	/** Compute steering input */
	float CalcSteeringInput(float DeltaTime);

	/** Compute brake input */
	float CalcBrakeInput()const;

	

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
	void UpdateWheelAnimation(float DeltaTime);
	void SimulateWheelData(float DeltaTime);
	EAIVehicleState DetermineAIState(float ForwardFactor,float DeltaTime);
	//trace and apply Suspension forces
	void WheelTrace(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel) const;
	//same as top function without force applying
	void SimulateWheel(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	//Apply Drive Brake and friction
	void ApplyWheelForces(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel);
	void CalculateSteeringAngle(FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel,float InNormSteering) ;
	static float GetSpringStiffness(FWheelState WheelState,float CompressionRatio);
	//AI movement
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	virtual void StopActiveMovement() override;
	
	//ChaosDefault
	static float CmToM(float In);
	///replication


	bool ShouldProcessPhysics()const;
	bool ShouldProcessCosmetics()const;
	bool ShouldProcessInput()const;
	bool CylinderTrace(UPrimitiveComponent* Shape,FVector Start,FVector End ,TArray<FHitResult>& result,const FComponentQueryParams& Params )const;

	/** Pack cosmetic data into optimized replicated variable */
	void UpdateReplicatedCosmeticData();

	/** Replciated cosmetic data  */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_RepCosmeticData)
	FRepCosmeticData RepCosmeticData;

	UFUNCTION()
    void OnRep_RepCosmeticData();
	
	/** Pass current state to server */
	UFUNCTION(reliable, server,WithValidation)
    void ServerUpdateState(uint16 InQuantizeInput);
	/** Contains: throttle (1), steering (2), handbrake(3). 
	*  3222 2222 1111 1111
	*/
	UPROPERTY(Transient)
	uint16 QuantizeInput;
	

	//Delegates
	UPROPERTY(BlueprintAssignable)
	FOnGearChange OnGearChange;
	//debug

	void ShowDebugInfo(class AHUD* HUD, class UCanvas* Canvas, const class FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);
	/** Draw 2D debug text graphs on UI for the wheels, suspension and other systems */
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static float CalcDialAngle(float CurrentValue, float MaxValue);
	static void DrawDial(UCanvas* Canvas, FVector2D Pos, float Radius, float CurrentValue, float MaxValue);
	// draw 2D debug line to UI canvas
	static void DrawLine2D(UCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, FColor Color, float Thickness = 1.f);
	

	
#endif

	
	
};

