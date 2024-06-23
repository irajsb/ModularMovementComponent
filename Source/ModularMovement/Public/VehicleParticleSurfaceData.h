// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Particles/ParticleEmitter.h"
#include "UObject/NoExportTypes.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
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

	UPROPERTY(EditAnywhere,Category = "Setup")
	TObjectPtr<USoundBase> Sound;

	
	
};
UCLASS(Blueprintable,BlueprintType)
class MODULARMOVEMENT_API UVehicleParticleSurfaceData : public UObject
{
	GENERATED_BODY()


	// Default Particle
	UPROPERTY(EditAnywhere,Category = "Setup")
	FSurfaceParticleCouple DefaultParticle;
	
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

	UPROPERTY(EditAnywhere,Category = "Audio")
    	float MinAngularSpeedInRadianAudio=1.f;
    
	UPROPERTY(EditAnywhere,Category = "Audio")
	float MaxAngularSpeedInRadianAudio=25.f;
	UPROPERTY(EditAnywhere,Category = "Audio")
	float InputRiseInterpolationSpeed=5.f;
	UPROPERTY(EditAnywhere,Category = "Audio")
	float InputFallInterpolationSpeed=1.f;
	// time to  keep emitter alive for so they finish up the last sprites 
	UPROPERTY(EditAnywhere,Category = "Setup")
	float CleanupTime=3.f;

	UPROPERTY(EditAnywhere,Category = "Setup")
	USoundAttenuation* SoundAttenuation;
	
public:
	void UpdateParticleForWheel(float DeltaTime,UModularWheel* Wheel);

	void OnSleep() const;
private:
	void HandleParticle(const UModularWheel* Wheel,const FSurfaceParticleCouple& ParticleCouple);
	void HandleNiagaraParticle(const UModularWheel* Wheel,const FSurfaceParticleCouple& ParticleCouple);
	void HandleSound(const UModularWheel* Wheel, const FSurfaceParticleCouple& ParticleCouple, float DeltaTime);

	UPROPERTY(Transient)
	USceneComponent* CurrentEmitter;

	UPROPERTY(Transient)
	UAudioComponent* AudioComponent;

	virtual void BeginDestroy() override;


	void AddCompForDelete(USceneComponent* SceneComponent);
	UPROPERTY(Transient)
	TArray<USceneComponent*>GarbageEmitters;
	void CleanupEmitters();
	FTimerHandle CleanupTimerHandle;
};
