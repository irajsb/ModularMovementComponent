// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "ModularVehicleFunctionLibrary.generated.h"

/**
 * 
 */
class UModularMovementComponent;
UCLASS()
class TITANMOVEMENT_API UModularVehicleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Do not use for audio without interpolation
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static float GetEngineRpm(UModularMovementComponent* MovementComponent);
	//Returns rpm ranged from 0-1 Do not use for audio without interpolation
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    static  float GetEngineRpmRatio(UModularMovementComponent* MovementComponent);
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    static  int GetCurrentGear(UModularMovementComponent* MovementComponent);
    UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
    static  int GetIdleGear(UModularMovementComponent* MovementComponent);
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetThrottleInputOnModularVehicle(APawn* Pawn,float Throttle);
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetSteerInputOnModularVehicle(APawn* Pawn,float Steer);
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetHandBrakeInputOnModularVehicle(APawn* Pawn,bool Brake);
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static FTransform GetWheelAnimationData(USceneComponent* Wheel);

	

	//not only for BP but we also store common functions here
	static float CalculateSuspensionRotationUsingPivot(UActorComponent* InComponent);
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", WorldContext="WorldContextObject", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "CapsuleTraceByChannel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
    static bool CapsuleTraceSingleWithRotation(UObject* WorldContextObject, const FVector Start, const FVector End,FRotator Rot, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	static FCollisionQueryParams ConfigureCollisionParams(FName TraceTag, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, bool bIgnoreSelf, UObject* WorldContextObject);

};
