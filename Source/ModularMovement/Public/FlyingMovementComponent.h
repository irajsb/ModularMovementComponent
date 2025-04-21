// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseModularMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "FlyingMovementComponent.generated.h"



DECLARE_LOG_CATEGORY_EXTERN(LogHeliMvmt, Log, All);


UENUM(BlueprintType)
enum class EChopperEngineState : uint8
{
	Off,
	SpoolingUp,
	Running,
	SpoolingDown,
};


USTRUCT(DisplayName="Rotor Setup")
struct MODULARMOVEMENT_API FMM_RotorSetup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category="Rotor Setup")
    FName BoneName = EName::None;

    UPROPERTY(EditAnywhere, Category="Rotor Setup")
    FVector TorqueNormal = FVector::UpVector;
};


/**
 * Movement component that simulates helicopter flight physics
 * Handles lift, tilt, rotation and movement based on helicopter controls
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UFlyingMovementComponent : public UBaseModularMovementComponent
{
    GENERATED_BODY()
using Self = UFlyingMovementComponent;

public:

	using TickFn = FActorComponentTickFunction;

	UFlyingMovementComponent();


	// Blueprint Configuration --------------------------------------------------

public:

	
	virtual void SetThrottleInput(float Input) override
	{
		SetCollectiveInput(Input);
	};

	virtual void SetSteeringInput(float Input) override
	{
		SetYawInput(Input);
	}
	
	
	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float RPM = 350;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float EnginePower = 400;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float SpoolUpTime = 10;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float CyclicSensitivity = 1;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float AntiTorqueSensitivity = 1;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float Agility = 1;

	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	TArray<FMM_RotorSetup> Rotors;


	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float LevitatingForceAlpha=0.9;

	//Force that keeps chopper upright
	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	float AntiRolloverForce=3000000000.f;

	/**
	 * X-Axis: Altitude (meters)
	 * Y-Axis: Main rotor effectiveness (0-1)
	 */
	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	TObjectPtr<UCurveFloat> AltitudePenaltyCurve;

	/**
	 * X-Axis: Angle of Attack (degrees)
	 * Y-Axis: Drag Coefficient
	 */
	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup")
	TObjectPtr<UCurveFloat> DragCoefficientCurve;

	/**
	 * When the aircraft is traveling at moderate airspeeds, the aerodynamic
	 * shape of the vehicle will tend to "pull" the vehicle's orientation to face
	 * the direction of travel. This curve determines the strength of that
	 * effect.
	 *
	 * X-Axis: Airspeed (m/s)
	 * Y-Axis: Influence of the torque applied by aerodynamics (0-1)
	 */
	UPROPERTY(EditDefaultsOnly, Category="VehicleSetup",
		DisplayName="Aerodynamic Torque Influence"
	)
	TObjectPtr<UCurveFloat> AeroTorqueInfluence;

	UPROPERTY(EditAnywhere, Category="Vehicle")
	bool DebugPhysics = false;


	// Blueprint Getters --------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetCurrentRPM() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetCurrentCollective() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	FVector2D GetCurrentCyclic() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetCurrentTorque() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	FVector GetVelocity() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetLateralAirspeed() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetLateralAirspeedKnots() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetVerticalAirspeed() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetHeadingDegrees() const;

	UFUNCTION(BlueprintPure, Category="Components|Movement|Heli")
	float GetRadarAltitude() const;


	// Blueprint Methods --------------------------------------------------------

	virtual void HoldStarter(float StartTime) override;

	virtual void StopEngine() override;
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Heli")
	void SetCollectiveInput(float value);

	UFUNCTION(BlueprintCallable, Category="Components|Movement|Heli")
	void SetPitchInput(float value);

	UFUNCTION(BlueprintCallable, Category="Components|Movement|Heli")
	void SetRollInput(float value);

	UFUNCTION(BlueprintCallable, Category="Components|Movement|Heli")
	void SetYawInput(float value);


	// Lifecycle & Events -------------------------------------------------------

public:

	void TickComponent(float deltaTime, ELevelTick type, TickFn* fn) override;


protected:

	FCalculateCustomPhysics OnCalculateCustomPhysics;

	void SetUpdatedComponent(USceneComponent* cmp) override;

	void SubstepTick(float deltaTime, FBodyInstance* body);

	virtual void UpdateEngineState(float deltaTime);
	virtual void UpdatePhysicsState(float deltaTime, FBodyInstance* body);
	virtual void UpdateSimulation(float deltaTime, FBodyInstance* body) const;
    void AddStabilizingTorque(FBodyInstance* Body, float DeltaTime) const;

private:

	// Data Structures ----------------------------------------------------------

	struct FInput
	{
		float Collective = 0;
		float Pitch = 0;
		float Roll = 0;
		float Yaw = 0;
	};

	
	// Helper for tracking engine state transitions during the "spooling" phases
	struct FEngineState
	{
	
		float SpoolAlpha = 0;
		float PowerAlpha = 0;
		float RPM = 0;
	};

	// Container for data read from the physics body
	struct FPhysicsState
	{
		float Mass = 0;
		float CrossSectionalArea = 0;
		/** In radians */
		float AngleOfAttack = 0;
		FVector COM = FVector::ZeroVector;
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocity = FVector::ZeroVector;
		FVector DeltaVelocity = FVector::ZeroVector;
		FVector GForce = FVector::ZeroVector;
	};

	// Details ------------------------------------------------------------------

	FInput m_Input;
	FEngineState m_EngineState;
	FPhysicsState m_PhysicsState;

	inline static float const k_Gravity = -981;
	inline static float const k_CmPerSecToKnots = 0.019438;

	APawn* GetPawn() const;
	FBodyInstance* GetBodyInstance() const;

	/**
	 * IMPORTANT: This should only be called from a FPhysicsCommand read/write
	 * callback with a locked mutex.
	 */
	float ComputeCrossSectionalArea(
		FBodyInstance const* body,
		FPhysicsActorHandle const& handle,
		FVector const& velocityDirection)
		const;

	FVector ComputeThrust(FVector const& pos, float mass) const;
	FVector ComputeDrag(FVector const& velocity, float aoa, float area) const;
	FVector ComputeTorque(FVector const& angularVelocity, float mass) const;
	void ComputeAeroTorque(FVector const& velocity, float mass, FVector& inout_torque) const;

	FVector Forward() const;
	FVector Right() const;
	FVector Up() const;

	void DebugPhysicsSimulation(
		FVector const& centerOfMass,
		FVector const& linearVelocity,
		FVector const& thrust,
		FVector const& drag,
		float crossSectionalArea)
		const;

	template <typename T>
	static FORCEINLINE T InverseLerp(T value, T min, T max)
	{
		return FMath::Clamp((value - min) / (max - min), 0, 1);
	}
	MIX_FLOATS_3_ARGS(InverseLerp);

	/** Fit a [0, 1] alpha value to a sine curve */
	template <typename T>
	static FORCEINLINE T CurveSin(T alpha)
	{
		if (alpha < 0.5) return -1.0 * FMath::Cos(alpha * UE_HALF_PI) + 1.0;
		return FMath::Sin(alpha * UE_HALF_PI);
	}


	//Packing input into a quat for net optimizations

	UFUNCTION(Server, Reliable)
	void SetInputOnServer(FVector4 Input);


	UPROPERTY(Replicated)
	EChopperEngineState EnginePhase = EChopperEngineState::Off;

	void SetEngineState(EChopperEngineState State);
	UFUNCTION(Server, Reliable)
	void SetEngineStateOnServer(EChopperEngineState State);

	FVector4 LastInput;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

};