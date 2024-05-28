// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ModularMovementPhysicalMaterial.generated.h"

/**
 * 
 */
UCLASS()
class MODULARMOVEMENT_API UModularMovementPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	//Drag this surface has on wheel
	UPROPERTY(EditAnywhere,Category="ModularPhysicsMaterial")
	float DragCoefficient=550.f;

	//Drag force on body
	UPROPERTY(EditAnywhere,Category="ModularPhysicsMaterial")
	float BodyDragCoefficient=500;
};
