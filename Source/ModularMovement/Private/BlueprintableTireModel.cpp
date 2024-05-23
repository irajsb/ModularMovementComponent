// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintableTireModel.h"

#include "ModularMovementComponent.h"
#include "ModularWheel.h"

void UBlueprintableTireModel::UpdateSimulation(float DeltaTime, FVector& FinalForceVector,
                                               UPrimitiveComponent* Mesh, UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel)
{
	Super::UpdateSimulation(DeltaTime, FinalForceVector, Mesh, ModularMovementComponent, Wheel);

	const FTransform WorldTransform = ModularMovementComponent->GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = Wheel->WheelState.SteerAngle;
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	FVector WorldMeshVelocity = ModularMovementComponent->GetMesh()->GetBodyInstance()->
															   GetUnrealWorldVelocityAtPoint(
																   Wheel->WheelState.HitResult.TraceStart);
	
	WorldMeshVelocity.Z = 0;
	const FVector LocalWheelVelocity = WorldTransform.InverseTransformVector(WorldMeshVelocity / 100.f);
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);

	
	
	HandleSimulation(DeltaTime,ModularMovementComponent,Wheel,GroundVelocityVector,FinalForceVector,Wheel->WheelState.AngularVelocity);
}

void UBlueprintableTireModel::SetupWheels()
{
	Super::SetupWheels();
}

float UBlueprintableTireModel::GetTireStress()
{
	return HandleGetTireStress();
}

void UBlueprintableTireModel::HandleSimulation_Implementation(float DeltaTime,
	UModularMovementComponent* ModularMovementComponent, UModularWheel* Wheel, FVector WheelSpaceVelocity,
	FVector& FinalForceVector, float& NewWheelAngularVelocity)
{
	UE_LOG(LogTemp,Log,TEXT("HandleSimulation not implemented in %s"),*GetName())
}

void UBlueprintableTireModel::HandleInitializeSimulation_Implementation()
{
}

float UBlueprintableTireModel::HandleGetTireStress_Implementation()
{
	return 0.f;
}

