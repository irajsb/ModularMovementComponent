//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "BaseVehicleData.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ModularMovementComponent.generated.h"


#define SIForceToUnrealForce(In) In*100.0f
class UModularWheel;
class UModularVehicleDebugger;
//Cosmetic delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGearChange, int, CurrentGear, int, TargetGear, bool, Finished);

struct FModularVehicleDebugParams
{
	int ShowSuspensionDebug = false;
	int ShowInputProcessingDebug = false;
	int ShowGearboxLog = false;
	int ShowDrawFriction = false;
	int AIDebug = false;
};


USTRUCT()
struct FOldRigidBodyErrorCorrection
{
	GENERATED_USTRUCT_BODY()

	/** max squared position difference to perform velocity adjustment */
	UPROPERTY()
	float LinearDeltaThresholdSq;

	/** strength of snapping to desired linear velocity */
	UPROPERTY()
	float LinearInterpAlpha;

	/** inverted duration after which linear velocity adjustment will fix error */
	UPROPERTY()
	float LinearRecipFixTime;

	/** max squared angle difference (in radians) to perform velocity adjustment */
	UPROPERTY()
	float AngularDeltaThreshold;

	/** strength of snapping to desired angular velocity */
	UPROPERTY()
	float AngularInterpAlpha;

	/** inverted duration after which angular velocity adjustment will fix error */
	UPROPERTY()
	float AngularRecipFixTime;

	/** min squared body speed to perform velocity adjustment */
	UPROPERTY()
	float BodySpeedThresholdSq;

	FOldRigidBodyErrorCorrection()
		: LinearDeltaThresholdSq(10.0f)
		, LinearInterpAlpha(0.1f)
		, LinearRecipFixTime(2.0f)
		, AngularDeltaThreshold(0.15f * PI)
		, AngularInterpAlpha(0.1f)
		, AngularRecipFixTime(2.0f)
		, BodySpeedThresholdSq(0.2f)
	{
	}
};

UENUM()
enum EAIVehicleState { /*target is in front*/Neutral,/*target is in back*/TurningAround };

USTRUCT()
struct FWheelRepCosmeticData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float Slip;
	UPROPERTY()
	float AngularVelocity;
	
};
USTRUCT()
struct FRepCosmeticData
{
	GENERATED_USTRUCT_BODY()

	/** Engine RPM */
	UPROPERTY()
	uint8 EngineRPM;
	UPROPERTY()
	uint8 CurrentGear = 0;
	UPROPERTY()
	float SteeringInput;
	UPROPERTY()
	TArray<FWheelRepCosmeticData> WheelRepCosmeticDatas;
	UPROPERTY()
	FRigidBodyState RigidBodyState;
	FRepCosmeticData()
	{
		EngineRPM = 0;
		CurrentGear = 0;
	}

};

USTRUCT()
struct FModularTrackInfo
{
	GENERATED_BODY()

	float TorqueTransfer;
};

// A structure representing the state of a vehicle
USTRUCT(BlueprintType)
struct FVehicleState
{
	GENERATED_BODY()

	// Class pointer to the vehicle data asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Setup)
	TSoftClassPtr<UBaseVehicleData> VehicleDataClass;

	// Pointer to the vehicle data
	UPROPERTY(BlueprintReadWrite, Category = MovementComponent)
	UBaseVehicleData* VehicleData;

	// Current RPM (Revolutions Per Minute) of the vehicle's engine
	UPROPERTY(BlueprintReadWrite, Category = MovementComponent)
	float CurrentRpm;

	// Forward speed of the vehicle
	UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
	float ForwardSpeed;

	// Side speed of the vehicle
	UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
	float SideSpeed;

	// Desired speed for the vehicle
	float DesiredSpeed;

	// Previous throttle input for AI control
	float AIPreviousThrottle;

	// State of the AI-controlled vehicle
	EAIVehicleState AIState;

	// Flag indicating whether the vehicle is controlled by AI
	bool IsAIVehicle;

	// Delta for locking the current state
	float LockCurrentStateDelta;

	// Number of drive wheels on the ground
	int DriveWheelsOnGround;

	// Information about the left track of the vehicle
	FModularTrackInfo TrackLeft;

	// Information about the right track of the vehicle
	FModularTrackInfo TrackRight;

	// Engine revolutions per second
	float EngineRads;
};


UCLASS(meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UModularMovementComponent : public UPawnMovementComponent
{
public:
	GENERATED_BODY()
	friend class UModularWheel;
	//Constructor
	UModularMovementComponent();

	//Gather wheels . Can be used to attach additional wheels such as semi truck trailers
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement", meta=(AutoCreateRefTerm="AdditionalWheels"))
	void UpdateComponents(TArray<UModularWheel*> AdditionalWheels);


	//actors to ignore for wheel trace. collected here because wheels can have different owners (such as a semi truck)
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

	//Wheels
	UPROPERTY()
	TArray<UModularWheel*> Components;

public:
	//Get List of wheels
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
	TArray<UModularWheel*> GetWheels();

	//Return Mesh
	UMeshComponent* GetMesh() const;


	// These properties represent various input values and constants for a vehicle.

	// Transient throttle input value (acceleration pedal)
	UPROPERTY(Transient, BlueprintReadOnly, Category = Input)
	float ThrottleInput;

	// Transient raw brake input value (brake pedal)
	UPROPERTY(Transient)
	float RawBrakeInput;

	// Transient raw steering input value (-1 for left, 1 for right)
	UPROPERTY(Transient)
	float RawSteeringInput;

	// Transient steering input value (-1 for left, 1 for right)
	UPROPERTY(Transient)
	float SteeringInput;

	// Transient brake input value
	UPROPERTY(Transient)
	float BrakeInput;

	// Transient raw throttle input value (-1 for release, 0 for neutral, 1 for full throttle)
	UPROPERTY(Transient)
	float RawThrottleInput;

	// Transient handbrake input status (true if engaged)
	UPROPERTY(Transient)
	bool HandBrakeInput;

	// Public properties representing air drag and rolling resistance constants for the vehicle.
	public:
	float AirDragConstant;
	float RollingResistanceConstant;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
	UBaseVehicleData* GetSetup() const;
	/*Set throttle Input
	 *Range -1,1
	 * No Need for replication
	 */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetThrottleInput(float Input);
	/*Set Steering Input
	*Range -1,1
	* No Need for replication
	*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetSteeringInput(float Input);
	/** Set the user input for the vehicle Brake [range 0 to 1] set it if you've turned off reverse as brake */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetBrakeInput(float Brake);
	/*Set HandBrakeInput
	* No Need for replication
	*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetHandBrakeInput(bool Brake);
	//Data holder
	UPROPERTY(Category=Setup, EditAnywhere, BlueprintReadOnly, meta=(ShowOnlyInnerProperties ))
	FVehicleState VehicleState;

	/** Compute steering input */
	float CalcSteeringInput(float DeltaTime);

	/** Compute brake input */
	float CalcBrakeInput() const;


	float CalcThrottleInput(float DeltaTime) const;
	//Engine


	//Get Number of wheels
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
	int GetNumberOfWheels() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
	int GetNumberOfDriveWheelsTouchingGround() const;
	//Get RPM is scale from 0-1
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
	float GetRPMRatio();


	//We setup wheels here
	virtual void InitializeComponent() override;


	void VehicleTick(float DeltaTime, FBodyInstance* BodyInstance);

	void PreTick(FPhysScene_Chaos* Scene, float DeltaTime);
	void PhysicsCallBack(float DeltaTime);
	class FModularAsyncCallBack* AsyncCallBack;

	//Main updates happen here
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	//Captures some basic info from wheels for feature processes 
	void CaptureState(float DeltaTime);

	//Calculate RPM And torque
	void UpdateEngine(float DeltaTime, float& WheelTorque);
	//Apply Air Drag
	void UpdateAirDrag() const;
	//Update each Wheel
	void UpdateWheels(float DeltaTime, float WheelTorque);
	//Determine vehicle state in AI Pawns

	EAIVehicleState DetermineAIState(float ForwardFactor, float DeltaTime);


	//AI movement
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	virtual void StopActiveMovement() override;


	//ChaosDefault
	static float CmToM(float In);

	///replication


	bool ShouldProcessPhysics() const;
	bool ShouldProcessCosmetics() const;
	bool ShouldReplicateInput() const;


	/** Pack cosmetic data into optimized replicated variable */
	void UpdateReplicatedCosmeticData();

	/** Replicated cosmetic data  */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_RepCosmeticData)
	FRepCosmeticData RepCosmeticData;



	UFUNCTION()
	void OnRep_RepCosmeticData();


	/** Pass current state to server */
	UFUNCTION(reliable, server, WithValidation)
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


	UPROPERTY()
	UModularVehicleDebugger* ModularVehicleDebugger;

	/** Correction thresholds cached info */
	FOldRigidBodyErrorCorrection ErrorCorrection;
	
	bool bRestoredState;
	bool bCorrectionInProgress;
	
	float CorrectionBeganTime;
	float CorrectionEndTime;
};
