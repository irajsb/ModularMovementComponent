//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once


#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ModuarVehicleDebugger.generated.h"

class UButtonWidgetStyle;
class UModularMovementComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGearboxDebugUpdate, int, Type, FString, Message);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNotifyEngineStatus, float, EngineTorque, float, WheelTorque, float,
                                               ThrottleInput);

/*
 * Attachable debugger for modular movement
 * Will spawn a widget that can debug various aspects of a vehicle
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UModularVehicleDebugger : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UModularVehicleDebugger();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;


	UPROPERTY(Transient)
	UModularMovementComponent* MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category=Debugger)
	FString GearBoxChangeGearAllowedStatus;
	UPROPERTY(BlueprintAssignable)
	FOnGearboxDebugUpdate OnGearboxDebugUpdate;
	UPROPERTY(BlueprintAssignable)
	FNotifyEngineStatus NotifyEngineStatus;

	float EngineTorque, WheelTorque, ThrottleInput;
};
