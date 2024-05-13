// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialParameterCollection.h"
#include "TerrainInteraction.generated.h"


class UModularMovementComponent;
class UModularWheel;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MODULARMOVEMENT_API UTerrainInteraction : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTerrainInteraction();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Update(float DeltaTime,UModularMovementComponent* MC, const TArray<UModularWheel*>& Components);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Simulating RT")
	UTextureRenderTarget2D* RenderTarget2D;



	//How big a RenderTarget pixel is in world
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Simulating RT")
	FVector2D PixelWorldSize;

	//The canvas size in world
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	FVector2D CanvasWorldSize = FVector2D(2048,2048);

	UPROPERTY(BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	FVector CanvasCenter;
	

	UPROPERTY(BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	float CanvasRot;
	

	UPROPERTY(EditAnywhere,Category = "World Drawing Board | Simulating RT")
	FVector2D BrushSize=FVector2D(32,64);

	UPROPERTY(EditAnywhere,Category = "World Drawing Board | Simulating RT")
	float PixelSizeScale=1.f;

	FVector2D  PreviousOffsetLocation;
	FVector2D  PreviousDrawLocation;

	// Sets PosAndSize Vector on this (naming should be exact)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	UMaterialParameterCollection* MaterialParameterCollection;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	UMaterialInterface* MaterialInterface;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	UMaterialInterface* OffsetMat;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "World Drawing Board | Canvas")
	float MinDrawDistance=30;
	
};
