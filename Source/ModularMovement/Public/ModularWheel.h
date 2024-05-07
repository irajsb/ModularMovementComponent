//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"

#include "ModularVehicleWheelData.h"
#include "TrackableComponent.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

#include "ModularWheel.generated.h"

class UVehicleParticleSurfaceData;
class UVehicleDebugWidget;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MODULARMOVEMENT_API UModularWheel : public UTrackableComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UModularWheel();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:	
	
	//Some Properties are not valid in SimulatedPawn
	UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ShowOnlyInnerProperties ),Category=Setup)
	FWheelState WheelState;
	UPROPERTY()
	FName OptionalBoneName;

	// override the parent that force is applied to
	UPROPERTY(BlueprintReadWrite,Transient,Category=Setup)
	UPrimitiveComponent* ParentBodyOverride;
	UPROPERTY(EditAnywhere,Category=Setup)
	uint8 DifferentialIndex=0;

	// For terrain interaction
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	bool AllowDrawInRenderTarget=true;
	//Surface emitter class
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category=Setup)
	TSubclassOf<UVehicleParticleSurfaceData> SurfaceDataClass;
	UPROPERTY(Transient)
	UVehicleParticleSurfaceData* SurfaceData;
	virtual void SetupWheels(UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSuspension(float DeltaTime,UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateForces(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual void UpdateSteering(float DeltaTime, UModularMovementComponent* ModularMovementComponent, float InNormSteering) ;
	virtual  void SetDriveTorqueOnWheels(float Force) ;
	virtual float GetFastestWheelOmegaSpeed() ;
	virtual void UpdateAnimation(float DeltaTime, UModularMovementComponent* ModularMovementComponent) ;
	virtual FTransform GetWheelTransform() ;

	 virtual void UpdateWheelState(FWheelState In) ;
	virtual FWheelState* GetWheelState() ;
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel")
	UModularVehicleWheelData* GetWheelSetup() const;

	
	///BP

	//Change Wheel Setup
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void UpdateWheelSetup(UModularVehicleWheelData* VehicleWheelData);
	//Update Wheel Steering 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	 void SetSteerOnWheel(float Angle );
	
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelRotation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelPivotRotation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelSteeringValue();
	//1 fully compressed 0 fully extended 
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelCompressionValue();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetWheelRPM();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	bool IsWheelTouchingGround();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	FVector GetWheelCenterLocation();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	float GetDampingForce();
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	float GetTireStress();

	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	float GetTrackSpeed() const;
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	 float GetTrackOffset(float CurrentOffset,float SpeedMultiplier) const;
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	UBaseTireModel* GetTireModel();


	


	//Set which differential index from array is active for this wheel 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	void SetActiveDifferentialIndex(uint8 Index,UModularMovementComponent* MovementComponent);

	//Get which differential index from array is active for this wheel 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleWheel",meta =(KeyWords="Set Change Update "))
	uint8 GetActiveDifferentialIndex();
	//Debug

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void ChangeTraceDebugVisibility(bool Enable);
	private:
	bool Debug=false;



	

	static Chaos::FRigidBodyHandle_Internal* GetInternalHandle(const UPrimitiveComponent* Component, FName BoneName);
	void AddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName);


	//Setup constraint only if suspension is of type constraint
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void SetupConstraints(UModularMovementComponent* MovementComponent,UPrimitiveComponent* ParentBody, UPrimitiveComponent* WheelOrDifferential,UPrimitiveComponent* InWheelCollision);
	/* you can create custom events using this. not replicated... its used to unify events an handle them once by reacting to them on modular movement OnCustomEventDelegate for example when tire is flattened
	 *You can call this event on a specific wheel but react to it and write code once for all wheels in modular movement OnCustomEventDelegate
	 * */
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void CallCustomEvent(uint8 Index);
	UPROPERTY()
	UPhysicsConstraintComponent* SuspensionConstraint=nullptr;
	UPROPERTY()
	UPrimitiveComponent* WheelCollision=nullptr;

	UPROPERTY()
	UPrimitiveComponent* ConstraintParent=nullptr;
	


	UPROPERTY()
	UPhysicalMaterial* NoFrictionDefaultPhysMaterial;

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<USceneComponent*> ChildWheels;

	UPROPERTY(Transient)
	UModularMovementComponent* MovementComponentRef;
};
