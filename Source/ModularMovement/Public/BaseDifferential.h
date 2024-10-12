// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseDifferential.generated.h"

class UModularWheel;
class UModularMovementComponent;

UCLASS(Blueprintable, BlueprintType)
class MODULARMOVEMENT_API ABaseDifferential : public AActor
{
	GENERATED_BODY()

	ABaseDifferential();

public:


	/** Please add a function description */
	UFUNCTION(BlueprintCallable,BlueprintImplementableEvent,Category="Differential")
	void Setup(UPrimitiveComponent* Mesh, UModularMovementComponent* MC, bool FromConstruct);

	/** Please add a function description */
	UFUNCTION(BlueprintCallable,BlueprintImplementableEvent,Category="Differential")
	void SetWheelOverrides();


	UFUNCTION(BlueprintCallable,BlueprintNativeEvent,Category="Differential")
	void  GetWheels(UModularWheel*& Left,UModularWheel*& Right);

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent,Category="Differential")
	void  GetWheelMeshes(UStaticMeshComponent*& Left,UStaticMeshComponent*& Right);
	
	
};
