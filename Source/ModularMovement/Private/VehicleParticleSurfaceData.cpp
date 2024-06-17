// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleParticleSurfaceData.h"

#include "ModularWheel.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Async/Async.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Audio/SoundParameterControllerInterface.h"
#include "Particles/ParticleSystemComponent.h"

void UVehicleParticleSurfaceData::UpdateParticleForWheel(float DeltaTime,UModularWheel* Wheel)
{

	
	bool Found=false;
	bool SoundFound=false;
	if(const auto PhysMat=Wheel->GetActivePhysicalMaterial())
	{
		for(const auto& Surface:SurfaceParticleCouples)
		{
			if(Surface.MaterialClass==PhysMat)
			{
				if(Surface.Particle)
				{
					HandleParticle(Wheel,Surface);
					Found=true;
				}else if (Surface.NiagaraSystem)
				{
					HandleNiagaraParticle(Wheel,Surface);
					Found=true;
				}
				if(Surface.Sound)
				{
					HandleSound(Wheel,Surface, DeltaTime);
					SoundFound=true;
				}
			}
		}
	}
	if(!Found)
	{
		if(DefaultParticle.Particle)
		{
			HandleParticle(Wheel,DefaultParticle);
		
		}else if (DefaultParticle.NiagaraSystem)
		{
			HandleNiagaraParticle(Wheel,DefaultParticle);
		}
		
	}
	if(!SoundFound)
	{
		if(DefaultParticle.Sound)
		{
			HandleSound(Wheel,DefaultParticle, DeltaTime);
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

void UVehicleParticleSurfaceData::OnSleep() const
{
	AsyncTask(ENamedThreads::GameThread,[this]()
	{
		if(!CurrentEmitter)
			return;
		if(const auto Comp=Cast<UParticleSystemComponent>(CurrentEmitter)){

			Comp->SetFloatParameter("Strength",0.f);
			Comp->SetFloatParameter("Stress",0.f);
			return;
		} if(const auto Comp=Cast<UNiagaraComponent>(CurrentEmitter))
		{
			Comp->SetFloatParameter("Strength",0.f);
			Comp->SetFloatParameter("Stress",0.f);
		}
		if(AudioComponent)
		{
			AudioComponent->SetFloatParameter("Strength",0.f);
			AudioComponent->SetFloatParameter("Stress",0.f);
		}
	});
	
	
}

void UVehicleParticleSurfaceData::HandleParticle(const UModularWheel* Wheel,const FSurfaceParticleCouple& ParticleCouple)
{

	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);
	
	

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	const float Stress=Wheel->WheelState.HitResult.bBlockingHit?Wheel->WheelState.TireStress:0;
	
	
	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
	
	if(const auto Comp=Cast<UNiagaraComponent>(CurrentEmitter))
	{
		GarbageEmitters.Add(MakeTuple(Comp, 0.f));
		Comp->SetFloatParameter("Strength",0.f);
		Comp->SetFloatParameter("Stress",0.f);
	}

	if(const auto Comp=Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		if(Comp->Template.Get()!=ParticleCouple.Particle)
		{
			//Comp->SetActive(false);
			GarbageEmitters.Add(MakeTuple(Comp, 0.f));
			Comp->SetFloatParameter("Strength",0.f);
			Comp->SetFloatParameter("Stress",0.f);
			CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ParticleCouple.Particle,Wheel->WheelState.HitResult.ImpactPoint);
			return;
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		Comp->SetWorldRotation(Wheel->GetComponentRotation());
		Comp->SetFloatParameter("Strength",Rate);
		Comp->SetFloatParameter("Stress",Stress);
		
		
		
		
		
	}else
	{
		CurrentEmitter=	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ParticleCouple.Particle,Wheel->WheelState.HitResult.ImpactPoint);
	}
	

	
}

void UVehicleParticleSurfaceData::HandleNiagaraParticle(const UModularWheel* Wheel,const FSurfaceParticleCouple& ParticleCouple)
{
	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadian, MaxAngularSpeedInRadian), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;
	const float Stress=Wheel->WheelState.HitResult.bBlockingHit?Wheel->WheelState.TireStress:0;

		Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
	
	if (const auto Comp = Cast<UParticleSystemComponent>(CurrentEmitter))
	{
		GarbageEmitters.Add(MakeTuple(Comp, 0.f));
		Comp->SetFloatParameter("Strength",0.f);
		Comp->SetFloatParameter("Stress",0.f);
	}

	if (const auto Comp = Cast<UNiagaraComponent>(CurrentEmitter))
	{
		if (Comp->GetAsset() != ParticleCouple.NiagaraSystem)
		{
			
			GarbageEmitters.Add(MakeTuple(Comp, 0.f));
			Comp->SetFloatParameter("Strength",0.f);
			Comp->SetFloatParameter("Stress",0.f);
			CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),ParticleCouple.NiagaraSystem ,Wheel->WheelState.HitResult.ImpactPoint);
			return;
			
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		Comp->SetWorldRotation(Wheel->GetComponentRotation());
		Comp->SetFloatParameter("Strength",Rate);
		Comp->SetFloatParameter("Stress",Stress);
	}
	else
	{	
		CurrentEmitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),ParticleCouple.NiagaraSystem ,Wheel->WheelState.HitResult.ImpactPoint);
	}

	
}

void UVehicleParticleSurfaceData::HandleSound(const UModularWheel* Wheel, const FSurfaceParticleCouple& ParticleCouple, float DeltaTime)
{

	
	const float Velocity=FMath::Abs(Wheel->WheelState.AngularVelocity);
	
	

	float Rate =Wheel->WheelState.HitResult.bBlockingHit?
		FMath::GetMappedRangeValueClamped(FVector2D(MinAngularSpeedInRadianAudio, MaxAngularSpeedInRadianAudio), FVector2D(0.0f, 1.0f), Velocity)
			:0.f;

	const float Stress=Wheel->WheelState.HitResult.bBlockingHit?Wheel->WheelState.TireStress:0;
	
	
	Rate=FMath::Max(Rate,Wheel->WheelState.TireStress);
	
	
	

	if(const auto Comp=Cast<UAudioComponent>(AudioComponent))
	{
		if(Comp->Sound.Get()!=ParticleCouple.Sound)
		{
			//Comp->SetActive(false);
			GarbageEmitters.Add(MakeTuple(Comp, 0.f));
			
			Comp->FadeOut(1,0.f);
			AudioComponent=	UGameplayStatics::SpawnSoundAtLocation(GetWorld(),ParticleCouple.Sound,Wheel->WheelState.HitResult.ImpactPoint);
			return;
		}
		Comp->SetWorldLocation(Wheel->WheelState.HitResult.ImpactPoint);
		Comp->SetWorldRotation(Wheel->GetComponentRotation());
		float CurrentRate=0.f;
		float CurrentStress=0.f;
		for(auto Param:	Comp->GetInstanceParameters())
		{
			if(Param.ParamName=="Strength")
			{
				CurrentRate=Param.FloatParam;
			}
			else if(Param.ParamName=="Stress")
			{
				CurrentStress=Param.FloatParam;
			}
		}
		
		Comp->SetFloatParameter("Strength",UKismetMathLibrary::FInterpTo_Constant(CurrentRate,Rate,DeltaTime,Rate>CurrentRate?InputRiseInterpolationSpeed:InputFallInterpolationSpeed) );
		Comp->SetFloatParameter("Stress",UKismetMathLibrary::FInterpTo_Constant(CurrentStress,Stress,DeltaTime,Stress>CurrentStress?InputRiseInterpolationSpeed:InputFallInterpolationSpeed) );
		
		
		
		
		
	}else
	{
		AudioComponent=	UGameplayStatics::SpawnSoundAtLocation(GetWorld(),ParticleCouple.Sound,Wheel->WheelState.HitResult.ImpactPoint);
	}
	

}

void UVehicleParticleSurfaceData::BeginDestroy()
{
	for (int Index = GarbageEmitters.Num() - 1; Index >= 0; Index--)
	{
		auto& Garbage = GarbageEmitters[Index];
	
			if (Garbage.Key)
			{
				Garbage.Key->DestroyComponent();
				GarbageEmitters.RemoveAtSwap(Index);
				
			}
	}

	if(CurrentEmitter)
	{
		CurrentEmitter->DestroyComponent();
	}
	if(AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}
	UObject::BeginDestroy();
}


