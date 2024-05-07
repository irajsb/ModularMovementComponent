// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Particles/ParticleEmitter.h"
#include "UObject/NoExportTypes.h"
#include "NiagaraSystem.h"
#include "VehicleParticleSurfaceData.generated.h"

/**
 * 
 */

class UModularWheel;

USTRUCT(BlueprintType)
struct FSurfaceParticleCouple
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UPhysicalMaterial* MaterialClass;

	UPROPERTY(EditAnywhere)
	UParticleSystem* Particle;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* NiagaraSystem;
	
};
UCLASS(Blueprintable,BlueprintType)
class MODULARMOVEMENT_API UVehicleParticleSurfaceData : public UObject
{
	GENERATED_BODY()

	//Default particle to use . Set one of the two defaults 
	UPROPERTY(EditAnywhere)
	UParticleSystem* DefaultParticle;
	//Default particle to use . Set one of the two defaults 
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* DefaultNiagaraSystem;
	UPROPERTY(EditAnywhere)
	TArray<FSurfaceParticleCouple> SurfaceParticleCouples;
	UPROPERTY(EditAnywhere)
	FVector SpawnOffset;
	UPROPERTY(EditAnywhere)
	FRotator RotOffset;
public:
	void UpdateParticleForWheel(UModularWheel* Wheel);

private:
	void HandleParticle(UModularWheel* Wheel,UParticleSystem* Particle);
	void HandleNiagaraParticle(UModularWheel* Wheel,UNiagaraSystem* Particle);


	UPROPERTY()
	UObject* CurrentEmitter;

	
};
