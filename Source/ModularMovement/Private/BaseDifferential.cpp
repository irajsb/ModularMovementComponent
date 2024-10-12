// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseDifferential.h"

// Sets default values
ABaseDifferential::ABaseDifferential()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void ABaseDifferential::GetWheelMeshes_Implementation(UStaticMeshComponent*& Left, UStaticMeshComponent*& Right)
{
}

void ABaseDifferential::GetWheels_Implementation(UModularWheel*& Left, UModularWheel*& Right)
{
	
}

