//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"

#include "CollisionQueryParams.h"
#include "ModularVehicleWheelData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/HitResult.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "ModularVehicleFunctionLibrary.generated.h"

/**
 * 
 */
class UModularWheel;
class UVehicleDebugWidget;
class UModularMovementComponent;

UCLASS(meta=(BlueprintThreadSafe))
class MODULARMOVEMENT_API UModularVehicleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Do not use for audio without interpolation
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static float GetEngineRpm(UModularMovementComponent* MovementComponent);
	//Returns rpm ranged from 0-1 Do not use for audio without interpolation
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static float GetEngineRpmRatio(UModularMovementComponent* MovementComponent);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static class UModularGearBox* GetGearBox(const UModularMovementComponent* MovementComponent);

public:
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetThrottleInputOnModularVehicle(APawn* Pawn, float Throttle);
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetSteerInputOnModularVehicle(APawn* Pawn, float Steer);
	/*Will ignore if movementComponent was not Modular Vehicle*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetHandBrakeInputOnModularVehicle(APawn* Pawn, bool Brake);


	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static int GetForwardSpeedKMH(UModularMovementComponent* MovementComponent);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static int GetForwardSpeedMPH(UModularMovementComponent* MovementComponent);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static float GetForwardSpeedCMs(UModularMovementComponent* MovementComponent);


	//Wheels
	//
	//
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement",
		meta=(KeyWords="Wheel,Num,Ground"))
	static int GetWheelsTouchingGround(UModularMovementComponent* MovementComponent);
	//not only for BP but we also store common functions here
	static float CalculateSuspensionRotationUsingPivot(UModularWheel* Wheel);
	UFUNCTION(BlueprintCallable, Category="Collision",
		meta=(bIgnoreSelf="true", WorldContext="WorldContextObject", AutoCreateRefTerm="ActorsToIgnore", DisplayName =
			"CapsuleTraceByChannel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	static bool CapsuleTraceSingleWithRotation(UObject* WorldContextObject, const FVector Start, const FVector End,
	                                           FRotator Rot, float Radius, float HalfHeight,
	                                           ETraceTypeQuery TraceChannel, bool bTraceComplex,
	                                           const TArray<AActor*>& ActorsToIgnore,
	                                           EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit,
	                                           bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red,
	                                           FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	static FCollisionQueryParams ConfigureCollisionParams(FName TraceTag, bool bTraceComplex,
	                                                      const TArray<AActor*>& ActorsToIgnore, bool bIgnoreSelf,
	                                                      UObject* WorldContextObject);
	//ADVANCED: Update wheel state on the wheel (Better to use a previous wheel state and update it and set it on the wheel ) 
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void UpdateWheelState(UModularWheel* Wheel, FModularWheelState NewWheelState);



	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void DrawDebugSphereWithDepth(UObject* Context, FVector Location, float Size, int Depth);


	//actual rotation does not give you acuurate steering and rotation values its has some corrections applied to it so get steering from dedicated values (additional data returned don't affect performance )
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void GetWheelAnimationData(UModularWheel* Wheel, FVector& Location, FRotator& Rotation, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	static void SetupWheelLocationFromBone(const USkeletalMeshComponent* Mesh,TArray<UTrackableComponent* >Wheels, const FString& BoneNamePrefix,FVector Offset);

	static void NotifyError(FString Error);
	UFUNCTION(BlueprintCallable, Category = "Game|Components|ModularVehicleMovement")
	void ChangeCollisionOnPhysicsBody(USkeletalMeshComponent* skeletalMesh, FName boneName, ECollisionEnabled::Type CollisionType);
};
