// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleParticleSurfaceData.h"

#include "ModularWheel.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
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

	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);
	
	

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
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
		Comp->SetFloatParameter("Strength",Rate);
		
		
		
		
		
	}else
	{
		CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Particle,Wheel->WheelState.HitResult.ImpactPoint);
	}
	

	
}

void UVehicleParticleSurfaceData::HandleNiagaraParticle(UModularWheel* Wheel, UNiagaraSystem* Particle)
{
	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
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
		Comp->SetFloatParameter("Strength",Rate);
	}
	else
	{	
		CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Particle ,Wheel->WheelState.HitResult.ImpactPoint);
	}

	
}
