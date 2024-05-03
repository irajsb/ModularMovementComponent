// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TerrainInteraction.generated.h"


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

	void Update(float DeltaTime, const TArray<UModularWheel*>& Components);
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTarget2D;

	// Limit to one draw per frame
	int ComponentDrawIndex=0;

	UPROPERTY()
	UCanvas* Canvas;
	
		
};
