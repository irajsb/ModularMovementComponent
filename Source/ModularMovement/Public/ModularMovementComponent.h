// Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only

#pragma once

#include "BaseVehicleData.h"
#include "ModularVehicleData.h"
#include "VehicleInputProcessor.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PawnMovementComponent.h"
#include "ModularMovementComponent.generated.h"

// Macro to convert SI force to Unreal force
#define SIForceToUnrealForce(In) In * 100.0f

class UTerrainInteraction;
class UModularWheel;
class UModularVehicleDebugger;

// Cosmetic delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGearChange, int, CurrentGear, int, TargetGear, bool, Finished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEngineStateChange, bool, IsEngineOn, bool, IsStarting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSleepChange, bool, Sleep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomEvent, uint8, EventType, UModularWheel*, Wheel);

// Debug parameters structure
struct FModularVehicleDebugParams
{
    // Show suspension debug information
    int ShowSuspensionDebug = false;
    // Show input processing debug information
    int ShowInputProcessingDebug = false;
    // Show gearbox log information
    int ShowGearboxLog = false;
    // Show draw friction information
    int ShowDrawFriction = false;
    // Show AI debug information
    int AIDebug = false;
};

// Enum for vehicle network mode
UENUM(BlueprintType)
enum EVehicleNetworkMode
{
    Default,
    ClientAuthoritative,
};

// Structure for old rigid body error correction
USTRUCT()
struct FOldRigidBodyErrorCorrection
{
    GENERATED_USTRUCT_BODY()

    // Maximum alpha value for error correction
    UPROPERTY(EditAnywhere, Category = Network)
    float MaxAlpha;

    // Minimum distance to fix error
    UPROPERTY(EditAnywhere, Category = Network)
    float MinDistanceToFix;

    // Maximum distance to fix error
    UPROPERTY(EditAnywhere, Category = Network)
    float MaxDistanceToFix;

    // Speed factor for error correction
    UPROPERTY(EditAnywhere, Category = Network)
    float SpeedFactor;

    // Maximum angular alpha value for error correction
    UPROPERTY(EditAnywhere, Category = Network)
    float MaxAngularAlpha;

    // Minimum angle to fix error
    UPROPERTY(EditAnywhere, Category = Network)
    float MinAngleToFix;

    // Maximum angle to fix error
    UPROPERTY(EditAnywhere, Category = Network)
    float MaxAngleToFix;

    FOldRigidBodyErrorCorrection() : MaxAlpha(0.5), MinDistanceToFix(0), MaxDistanceToFix(2000), SpeedFactor(0.001),
                                     MaxAngularAlpha(0.1), MinAngleToFix(0), MaxAngleToFix(1.1775)
    {
    }
};

// Enum for AI vehicle state
UENUM()
enum EAIVehicleState { Neutral, TurningAround };

// Structure for wheel representation cosmetic data
USTRUCT()
struct FWheelRepCosmeticData
{
    GENERATED_USTRUCT_BODY()

public:
    // Slip value for the wheel
    UPROPERTY()
    float Slip = 0.f;

    // Angular velocity of the wheel
    UPROPERTY()
    float AngularVelocity = 0.f;

    FWheelRepCosmeticData(float InSlip, float InAngularVelocity)
    {
        Slip = InSlip;
        AngularVelocity = InAngularVelocity;
    }

    FWheelRepCosmeticData()
    {
        Slip = 0.f;
        AngularVelocity = 0.f;
    };
};

// Structure for representation cosmetic data
USTRUCT()
struct FRepCosmeticData
{
    GENERATED_USTRUCT_BODY()

    // Engine RPM
    UPROPERTY()
    uint8 EngineRPM = 0.f;

    // Current gear
    UPROPERTY()
    uint8 CurrentGear = 0;

    // Steering input
    UPROPERTY()
    float SteeringInput = 0.f;

    // Array of wheel representation cosmetic data
    UPROPERTY()
    TArray<FWheelRepCosmeticData> WheelRepCosmeticDatas;

    // Rigid body state
    UPROPERTY()
    FRigidBodyState RigidBodyState;

    // Current fuel level
    UPROPERTY()
    float CurrentFuel;

    // Engine state
    UPROPERTY()
    bool EngineOn;
    UPROPERTY()
    bool IsSleep;

    FRepCosmeticData() : CurrentFuel(0), EngineOn(0)
    {
        EngineRPM = 0;
        CurrentGear = 0;
    }
};

// Structure for modular track information
USTRUCT()
struct FModularTrackInfo
{
    GENERATED_BODY()

    // Torque transfer for the track
    float TorqueTransfer;
};

// Structure representing the state of a vehicle
USTRUCT(BlueprintType)
struct FVehicleState
{
    GENERATED_BODY()

    // Class pointer to the vehicle data asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Setup)
    TSoftClassPtr<UBaseVehicleData> VehicleDataClass;

    // Pointer to the vehicle data
    UPROPERTY(BlueprintReadWrite, Category = MovementComponent)
    UBaseVehicleData* VehicleData = nullptr;

    // Current RPM (Revolutions Per Minute) of the vehicle's engine
    UPROPERTY(BlueprintReadWrite, Category = MovementComponent)
    float CurrentRpm = 0.f;

    // Forward speed of the vehicle
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float ForwardSpeed = 0.f;

    // Side speed of the vehicle
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float SideSpeed = 0.f;

    // Wheel torque
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float WheelTorque;

    // Desired speed for the vehicle
    float DesiredSpeed = 0.f;

    // Previous throttle input for AI control
    float AIPreviousThrottle = 0.f;

    // State of the AI-controlled vehicle
    EAIVehicleState AIState;

    // Flag indicating whether the vehicle is controlled by AI
    bool IsAIVehicle = false;

    // Delta for locking the current state
    float LockCurrentStateDelta = 0.f;

    // Number of drive wheels on the ground
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    int DriveWheelsOnGround = 0;

    // Number of wheels on the ground
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    int WheelsOnGround = 0;

    // Information about the left track of the vehicle
    FModularTrackInfo TrackLeft;

    // Information about the right track of the vehicle
    FModularTrackInfo TrackRight;

    // Engine revolutions per second
    float EngineRads = 0.f;

    // Current fuel level
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float CurrentFuel = 0.f;

    // Engine state
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    bool IsEngineOn = false;

    // Sleep state
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    bool bSleeping = false;
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float SleepTimer=0.f;

    // Axle RPM
    UPROPERTY(BlueprintReadOnly, Category = MovementComponent)
    float AxleRPM = 0.f;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UModularMovementComponent : public UPawnMovementComponent
{
public:
    GENERATED_BODY()
    friend class UModularWheel;

    // Constructor
    UModularMovementComponent();

    // Gather wheels. Can be used to attach additional wheels such as semi truck trailers
    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement", meta = (AutoCreateRefTerm = "AdditionalWheels"))
    void UpdateComponents(TArray<UModularWheel*> AdditionalWheels);

    // Actors to ignore for wheel trace. Collected here because wheels can have different owners (such as a semi truck)
    UPROPERTY(BlueprintReadWrite, Category = "Trace")
    TArray<AActor*> ActorsToIgnore;

    // Wheels
    UPROPERTY()
    TArray<UModularWheel*> Components;

public:
    // Get List of wheels
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    TArray<UModularWheel*> GetWheels();

    // Return Mesh
    UMeshComponent* GetMesh() const;

    // Input values and constants for a vehicle
    UPROPERTY(Transient, BlueprintReadOnly, Category = Input)
    float ThrottleInput;

    UPROPERTY(Transient)
    float RawBrakeInput;

    UPROPERTY(Transient)
    float RawSteeringInput;

    UPROPERTY(Transient)
    float SteeringInput;

    UPROPERTY(Transient,BlueprintReadOnly,Category=Input)
    float BrakeInput;
    UPROPERTY(Transient,BlueprintReadOnly,Category=Input)
    bool IsBraking;

    UPROPERTY(Transient, BlueprintReadOnly, Category = Input)
    float RawThrottleInput;

    UPROPERTY(Transient)
    bool HandBrakeInput;

    UPROPERTY(Transient)
    float CurrentDifferentialRatio;

    // Air drag and rolling resistance constants for the vehicle
    float AirDragConstant;
    bool UseCustomDrag = false;
    float CustomDragCoefficient = 0.f;
    float RollingResistanceConstant;

    // Get vehicle setup
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    UBaseVehicleData* GetSetup() const;

    // Set input functions
    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetThrottleInput(float Input);

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetSteeringInput(float Input);

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetBrakeInput(float Brake);

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetHandBrakeInput(bool Brake);

    // Data holder
    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly, meta = (ShowOnlyInnerProperties))
    FVehicleState VehicleState;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool ApplyRecommendedMeshProperties = true;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    float SleepDelay=0.4;
    
    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool AllowSleep = true;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool AllowTankSleep = false;

    
    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool SpawnWithTurnedOffEngine = false;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    TEnumAsByte<EVehicleNetworkMode> NetworkMode;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool SubstepEngine = true;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool SubStepSuspension = true;

    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly)
    bool IsTrailer;
   
    UPROPERTY(Category = Setup, EditAnywhere, BlueprintReadOnly,AdvancedDisplay)
    TSubclassOf<UVehicleInputProcessor> InputProcessor=UVehicleInputProcessor::StaticClass();
    
    
    // Engine functions
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    int GetNumberOfWheels() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    int GetNumberOfDriveWheelsTouchingGround() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    float GetRPMRatio();

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void HoldStarter(float StartTime);

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void ReleaseStarter();

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void StopEngine();

    // Fuel functions
    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void AddFuel(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    void SetFuel(float Amount);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game|Components|ModularVehicleMovement")
    float GetFuelRatio();

    FTimerHandle StarterTimerHandle;

    // Component initialization
    virtual void InitializeComponent() override;

    // Tick functions
    void VehicleTick(float DeltaTime, bool SubstepTick);
    void PreTick(FPhysScene_Chaos* Scene, float DeltaTime);
    void PhysicsCallBack(float DeltaTime);

    class FModularAsyncCallBack* AsyncCallBack;

    // Main updates happen here
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;

    // Capture state and update functions
    void CaptureState(float DeltaTime);
    void UpdateEngine(float DeltaTime, float& WheelTorque);
    void UpdateAirDrag(UPrimitiveComponent * CompToApplyForceTo) const;
    void UpdateTankSteering(float UseSteeringValue);
    void UpdateWheels(float DeltaTime, float WheelTorque, bool SubstepTick);

    // AI movement functions
    EAIVehicleState DetermineAIState(float ForwardFactor, float DeltaTime);
    virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
    virtual void StopActiveMovement() override;

    // ChaosDefault function
    static float CmToM(float In);

    // Replication functions
    bool ShouldProcessPhysics();
    bool ShouldProcessCosmetics();
    bool ShouldReplicateInput() const;
    void UpdateReplicatedCosmeticData();

    // Replicated cosmetic data
    UPROPERTY(Transient, ReplicatedUsing = OnRep_RepCosmeticData)
    FRepCosmeticData RepCosmeticData;

    bool CosmeticDataInitialized = false;

    UFUNCTION()
    void OnRep_RepCosmeticData();

    UFUNCTION(Server, Reliable)
    void SetCosmeticDataOnServer(FRepCosmeticData Data);

    UFUNCTION(reliable, server, WithValidation)
    void ServerUpdateState(uint16 InQuantizeInput);

    UPROPERTY(Transient)
    uint16 QuantizeInput;

    // Delegates
    UPROPERTY(BlueprintAssignable)
    FOnGearChange OnGearChange;

    UPROPERTY(BlueprintAssignable)
    FOnSleepChange OnSleepChange;

    UPROPERTY(BlueprintAssignable)
    FOnEngineStateChange OnEngineStateChange;

    // Debug
    UPROPERTY()
    UModularVehicleDebugger* ModularVehicleDebugger;

    UPROPERTY(EditAnywhere, Category = Network)
    FOldRigidBodyErrorCorrection ErrorCorrection;

    void ApplyDifferential(FDifferentialData DiffData, float EngineTorque, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Sleep")
    void SetSleeping(bool bEnableSleep);

    static void ShowSetupError(FString Error);

    FRigidBodyState NewestBodyInstance;

    void ApplyBodyInstanceData() ;

    UPROPERTY(BlueprintAssignable)
    FOnCustomEvent OnCustomEvent;

    bool IsLocal();

    TOptional<bool> CachedIsLocal;
    TOptional<bool> CachedShouldProcessPhysics;
    TOptional<bool> CachedShouldProcessCosmetics;

    float GetMassPerWheel() const;

    UFUNCTION(BlueprintCallable, Category = "Sleep")
    static void SetSleepOnBody(UPrimitiveComponent* PrimitiveComponent, bool Sleep);

    UFUNCTION()
    void HandleComponentWake(UPrimitiveComponent* WakingComponent, FName BoneName)
    {
        SetSleeping(false);
    }
};
