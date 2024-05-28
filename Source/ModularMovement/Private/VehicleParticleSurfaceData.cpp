// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleParticleSurfaceData.h"

#include "ModularWheel.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Particles/ParticleSystemComponent.h"

void UVehicleParticleSurfaceData::UpdateParticleForWheel(float DeltaTime,UModularWheel* Wheel)
{

	
	bool Found=false;
	if(const auto PhysMat=Wheel->GetActivePhysicalMaterial())
	{
		for(const auto Surface:SurfaceParticleCouples)
		{
			if(Surface.MaterialClass==PhysMat)
			{
				if(Surface.Particle)
				{
					HandleParticle(Wheel,Surface.Particle);
					Found=true;
				}else if (Surface.NiagaraSystem)
				{
					HandleNiagaraParticle(Wheel,Surface.NiagaraSystem);
					Found=true;
				}
			}
		}
	}
	if(!Found)
	{
		if(DefaultParticle)
		{
			HandleParticle(Wheel,DefaultParticle);
		
		}else if (DefaultNiagaraSystem)
		{
			HandleNiagaraParticle(Wheel,DefaultNiagaraSystem);
		}
	}

	//Update garbage emitters

	for (int Index = GarbageEmitters.Num() - 1; Index >= 0; Index--)
	{
		auto& Garbage = GarbageEmitters[Index];
		if (Garbage.Value > CleanupTime)
		{
			if (Garbage.Key)
			{
				Garbage.Key->DestroyComponent();
				GarbageEmitters.RemoveAtSwap(Index);
				
			}
		}
		else
		{
			Garbage.Value += DeltaTime;
		}
	}
}

void UVehicleParticleSurfaceData::HandleParticle(const UModularWheel* Wheel, UParticleSystem* Particle)
{

	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);
	
	

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
	if(const auto Comp=Cast<UNiagaraComponent>(CurrentEmitter))
	{
		GarbageEmitters.Add(MakeTuple(Comp, 0.f));
		Comp->SetFloatParameter("Strength",0.f);
	}

	if(const auto Comp=Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		if(Comp->Template.Get()!=Particle)
		{
			//Comp->SetActive(false);
			GarbageEmitters.Add(MakeTuple(Comp, 0.f));
			Comp->SetFloatParameter("Strength",0.f);
			CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Particle,Wheel->WheelState.HitResult.ImpactPoint);
			return;
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		Comp->SetFloatParameter("Strength",Rate);
		
		
		
		
		
	}else
	{
		CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Particle,Wheel->WheelState.HitResult.ImpactPoint);
	}
	

	
}

void UVehicleParticleSurfaceData::HandleNiagaraParticle(const UModularWheel* Wheel, UNiagaraSystem* Particle)
{
	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
	if (const auto Comp = Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		GarbageEmitters.Add(MakeTuple(Comp, 0.f));
		Comp->SetFloatParameter("Strength",0.f);
	}

	if (const auto Comp = Cast<UNiagaraComponent>(CurrentEmitter))
	{
		if (Comp->GetAsset() != Particle)
		{
			
			GarbageEmitters.Add(MakeTuple(Comp, 0.f));
			Comp->SetFloatParameter("Strength",0.f);
			CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Particle ,Wheel->WheelState.HitResult.ImpactPoint);
			return;
			
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		Comp->SetFloatParameter("Strength",Rate);
	}
	else
	{	
		CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Particle ,Wheel->WheelState.HitResult.ImpactPoint);
	}

	
}
