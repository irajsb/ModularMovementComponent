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

	UPROPERTY(EditAnywhere,Category = "Setup")
	UPhysicalMaterial* MaterialClass=nullptr;

	UPROPERTY(EditAnywhere,Category = "Setup")
	UParticleSystem* Particle=nullptr;

	UPROPERTY(EditAnywhere,Category = "Setup")
	UNiagaraSystem* NiagaraSystem=nullptr;
	
};
UCLASS(Blueprintable,BlueprintType)
class MODULARMOVEMENT_API UVehicleParticleSurfaceData : public UObject
{
	GENERATED_BODY()

	//Default particle to use . Set one of the two defaults 
	UPROPERTY(EditAnywhere,Category = "Setup")
	UParticleSystem* DefaultParticle=nullptr;
	//Default particle to use . Set one of the two defaults 
	UPROPERTY(EditAnywhere,Category = "Setup")
	UNiagaraSystem* DefaultNiagaraSystem=nullptr;
	UPROPERTY(EditAnywhere,Category = "Setup")
	TArray<FSurfaceParticleCouple> SurfaceParticleCouples;
	UPROPERTY(EditAnywhere,Category = "Setup")
	FVector SpawnOffset;
	UPROPERTY(EditAnywhere,Category = "Setup")
	FRotator RotOffset;

	UPROPERTY(EditAnywhere,Category = "Setup")
	float MinAngularSpeedInRadian=1.f;

	UPROPERTY(EditAnywhere,Category = "Setup")
	float MaxAngularSpeedInRadian=5.f;
	
public:
	void UpdateParticleForWheel(UModularWheel* Wheel);

private:
	void HandleParticle(UModularWheel* Wheel,UParticleSystem* Particle);
	void HandleNiagaraParticle(UModularWheel* Wheel,UNiagaraSystem* Particle);


	UPROPERTY()
	UObject* CurrentEmitter;

	
};
