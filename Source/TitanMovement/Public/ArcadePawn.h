// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "GameFramework/Pawn.h"
#include "ArcadePawn.generated.h"
class UModularMovementComponent;
UCLASS()
class TITANMOVEMENT_API AArcadePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AArcadePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UModularMovementComponent* ArcadeMovementComponent;

	//Get Car body
	UFUNCTION(BlueprintNativeEvent)
	UMeshComponent* GetSimulatedMesh();

};
