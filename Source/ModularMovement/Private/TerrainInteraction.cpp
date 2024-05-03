// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainInteraction.h"

#include "CanvasTypes.h"
#include "ModularMovement.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values for this component's properties
UTerrainInteraction::UTerrainInteraction()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTerrainInteraction::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTerrainInteraction::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTerrainInteraction::Update(float DeltaTime, const TArray<UModularWheel*>& Components)
{

	

}

