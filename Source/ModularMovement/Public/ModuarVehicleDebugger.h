// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "SlateTypes.h"
#include "SlateWidgetStyleAsset.h"
#include "Components/ActorComponent.h"
#include "ModuarVehicleDebugger.generated.h"

class UButtonWidgetStyle;
class UModularMovementComponent;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MODULARMOVEMENT_API UModuarVehicleDebugger : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UModuarVehicleDebugger();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Transient)
	UModularMovementComponent* MovementComponent;


		
};
