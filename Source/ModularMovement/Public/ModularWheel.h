// Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only

#pragma once

#include "CoreMinimal.h"
#include "ModularVehicleWheelData.h"
#include "TrackableComponent.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "ModularWheel.generated.h"

class UVehicleParticleSurfaceData;
class UVehicleDebugWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UModularWheel : public UTrackableComponent
{
    GENERATED_BODY()

public:
    UModularWheel();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Wheel setup properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), Category=Setup)
    FModularWheelState WheelState;

    UPROPERTY()
    FName OptionalBoneName;

    UFUNCTION(BlueprintCallable,Category=Effects)
    void DisableSurfaceEffects();
    UPROPERTY(BlueprintReadWrite, Transient, Category=Setup)
    UPrimitiveComponent* ParentBodyOverride;

    UPROPERTY(EditAnywhere, Category=Setup)
    uint8 DifferentialIndex = 0;

    UPROPERTY(EditAnywhere, Category=Setup)
    TArray<uint8> DifferentialBlackList;

    // Surface interaction properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Setup)
    bool AllowDrawInRenderTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Setup)
    TSubclassOf<UVehicleParticleSurfaceData> SurfaceDataClass;

    UPROPERTY(Transient)
    UVehicleParticleSurfaceData* SurfaceData;

    // Wheel simulation methods
    virtual void SetupWheels(UModularMovementComponent* ModularMovementComponent);
    virtual void UpdateSuspension(float DeltaTime, UModularMovementComponent* ModularMovementComponent);
    virtual void UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent);
    virtual void UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent, float InNormSteering);
    virtual void SetDriveTorqueOnWheels(float Force);
    virtual float GetFastestWheelOmegaSpeed();
    virtual void UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent);
    virtual FTransform GetWheelTransform();

    // Wheel state methods
    virtual void UpdateWheelState(FModularWheelState In);
    virtual FModularWheelState* GetWheelState();

    // Blueprint callable methods
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel")
    UModularVehicleWheelData* GetWheelSetup() const;

    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel", meta=(KeyWords="Set Change Update"))
    void UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData);
    // only useful when called in construction script ;
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel", meta=(KeyWords="Set Change Update"))
    void SetupWheelClass(TSoftClassPtr<UModularVehicleWheelData> WheelSetupClass);
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel", meta=(KeyWords="Set Change Update"))
    void SetSteerOnWheel(float Angle);

    // Blueprint pure methods
    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetWheelRotation();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetWheelPivotRotation();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetWheelSteeringValue();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetWheelCompressionValue();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetWheelRPM();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    bool IsWheelTouchingGround();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    FVector GetWheelCenterLocation();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetDampingForce();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetTireStress();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetTrackSpeed() const;

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    float GetTrackOffset(float CurrentOffset, float SpeedMultiplier) const;

    UFUNCTION(BlueprintPure, BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    UBaseTireModel* GetTireModel();

    // Differential methods
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel", meta=(KeyWords="Set Change Update"))
    void SetActiveDifferentialIndex(uint8 Index, UModularMovementComponent* MovementComponent);

    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleWheel", meta=(KeyWords="Set Change Update"))
    uint8 GetActiveDifferentialIndex();

    // Debug methods
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    void ChangeTraceDebugVisibility(bool Enable);

private:
    bool Debug = false;

    // Internal methods
    static Chaos::FRigidBodyHandle_Internal* GetInternalHandle(const UPrimitiveComponent* Component, FName BoneName);
    void AddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName);

    // Constraint setup methods
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    void SetupConstraints(UModularMovementComponent* MovementComponent, UPrimitiveComponent* ParentBody, UPrimitiveComponent* WheelOrDifferential, UPrimitiveComponent* InWheelCollision, UPhysicsConstraintComponent* InOptionalConstraint, bool IsAxle);

    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    void SetupCustomConstraint(UModularMovementComponent* MovementComponent, UPhysicsConstraintComponent* ConstraintComponent, UPrimitiveComponent* ParentBody, UPrimitiveComponent* WheelOrDifferential, UPrimitiveComponent* InWheelCollision);

    // Custom event methods
    UFUNCTION(BlueprintCallable, Category="Game|Components|ModularVehicleMovement")
    void CallCustomEvent(uint8 Index);

public:
    // Public properties
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Game|Components|ModularVehicleMovement")
    UPhysicalMaterial* GetActivePhysicalMaterial();

    UPROPERTY(BlueprintReadOnly, Category=Constraint)
    UPhysicsConstraintComponent* SuspensionConstraint = nullptr;

    UPROPERTY(BlueprintReadOnly,Category=Wheel)
    UPrimitiveComponent* WheelCollision = nullptr;

    UPROPERTY(BlueprintReadOnly, Category=Constraint)
    UPrimitiveComponent* ConstraintParent = nullptr;

private:
    // Private properties
    UPROPERTY()
    UPhysicalMaterial* NoFrictionDefaultPhysMaterial;

public:
    // Public properties
    UPROPERTY(BlueprintReadOnly, Category=Wheel)
    TArray<USceneComponent*> ChildWheels;

    UPROPERTY(Transient)
    UModularMovementComponent* MovementComponentRef;

    UPROPERTY(Transient, BlueprintReadWrite, Category=Wheel)
    UPhysicalMaterial* PhysicalMaterialOverride;

    UPROPERTY()
    FVector TotalForces;

    void ApplyAccumulatedForces();


    UFUNCTION(BlueprintCallable,Category=ConstraintWheels)
    void ToggleAxleSuspensionXYLock(bool NewLock);
};
