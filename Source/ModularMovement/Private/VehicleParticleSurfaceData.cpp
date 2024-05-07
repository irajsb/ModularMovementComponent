// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleParticleSurfaceData.h"

#include "ModularWheel.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void UVehicleParticleSurfaceData::UpdateParticleForWheel(UModularWheel* Wheel)
{

	bool Found=false;
	if(Wheel->WheelState.HitResult.PhysMaterial.IsValid())
	{
		for(auto Surface:SurfaceParticleCouples)
		{
			if(Surface.MaterialClass==Wheel->WheelState.HitResult.PhysMaterial.Get())
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
}

void UVehicleParticleSurfaceData::HandleParticle(UModularWheel* Wheel, UParticleSystem* Particle)
{

	
	if(const auto Comp=Cast<UNiagaraComponent>(CurrentEmitter))
	{
		Comp->DestroyComponent();
	}

	if(const auto Comp=Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		if(Comp->Template.Get()!=Particle)
		{
			Comp->SetTemplate(Particle);
			
			
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		
		
		const bool ValidHit=Wheel->WheelState.HitResult.bBlockingHit;
		
		
		
	}else
	{
		CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Particle,Wheel->WheelState.HitResult.ImpactPoint);
	}
	

	
}

void UVehicleParticleSurfaceData::HandleNiagaraParticle(UModularWheel* Wheel, UNiagaraSystem* Particle)
{
	if (const auto Comp = Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		Comp->DestroyComponent();
	}

	if (const auto Comp = Cast<UNiagaraComponent>(CurrentEmitter))
	{
		if (Comp->GetAsset() != Particle)
		{
			Comp->SetAsset(Particle);
			
			
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		const bool ValidHit=Wheel->WheelState.HitResult.bBlockingHit;
		
		if(Comp->IsActive()!=ValidHit){
			Comp->SetActive(ValidHit,true);
		}
	}
	else
	{
		CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Particle ,Wheel->WheelState.HitResult.ImpactPoint);
	}

	
}
