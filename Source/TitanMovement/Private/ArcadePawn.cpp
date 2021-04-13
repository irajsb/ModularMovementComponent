// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadePawn.h"
#include "ArcadeMovementComponent.h"
// Sets default values
AArcadePawn::AArcadePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ArcadeMovementComponent=CreateDefaultSubobject<UArcadeMovementComponent>(TEXT("ArcadeMovementComponent"));
	ArcadeMovementComponent->UpdatedComponent=RootComponent;
}

// Called when the game starts or when spawned
void AArcadePawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AArcadePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AArcadePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

UMeshComponent* AArcadePawn::GetSimulatedMesh_Implementation()
{
	return Cast<UMeshComponent>(GetRootComponent());	
}

