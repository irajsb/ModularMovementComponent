//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "VehicleAudioComponent.h"

#include "ModularGearBox.h"
#include "ModularMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Kismet/GameplayStatics.h"

void UVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	TempEngineSound=Sound;
	if(const auto Pawn=Cast<APawn>(GetOwner()))
	{
		if(const auto MC= Cast<UModularMovementComponent>( Pawn->GetMovementComponent()))
		{
			MC->OnEngineStateChange.AddDynamic(this,&UVehicleAudioComponent::UVehicleAudioComponent::OnEngineStateChange);
		}
	}
	
}

UVehicleAudioComponent::UVehicleAudioComponent(): Load(0), CurrentTurbo(0)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate= false;
}

void UVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(const auto Pawn=Cast<APawn>(GetOwner()))
	{

		if(const auto MC= Cast<UModularMovementComponent>( Pawn->GetMovementComponent()))
		{
			
				const auto RPMRatio=MC->VehicleState.CurrentRpm/MC->VehicleState.VehicleData->GetMaxRPM();
				const float RPMChange=RPMRatio-RPM;
				const bool GearChanging=MC->GetSetup()->GetGearBox()->IsChangingGear();
				RPM=UKismetMathLibrary::FInterpTo_Constant(RPM,RPMRatio,DeltaTime,RPMInterpolationSpeed);
				SetFloatParameter("RPM",RPM*RPMMultiplier);

				const float NewLoad=GearChanging?0.f:((FMath::Abs(MC->ThrottleInput)/2)+(RPMChange > 0.05) )? 0.5f : 0.0f;
				Load=UKismetMathLibrary::FInterpTo_Constant(Load,NewLoad,DeltaTime,LoadInterpolationSpeed);
				SetFloatParameter("Load",Load*LoadMultiplier);

			
				CurrentTurbo=UKismetMathLibrary::FInterpTo_Constant(CurrentTurbo,RPMRatio,DeltaTime,TurboInterpolationSpeed);
				SetFloatParameter("Turbo",CurrentTurbo*TurboMultiplier);

			if(LastHandBrakeInput!=MC->HandBrakeInput)
			{
				LastHandBrakeInput=MC->HandBrakeInput;
				if(LastHandBrakeInput)
				{
					UGameplayStatics::PlaySoundAtLocation(GetWorld(),HandBrakeSound,GetComponentLocation());
				}else
				{
					UGameplayStatics::PlaySoundAtLocation(GetWorld(),HandReleaseSound,GetComponentLocation());
				}
			}
		}
	}
}

void UVehicleAudioComponent::OnEngineStateChange(bool IsEngineOn, bool IsStarting)
{
	if(!IsEngineOn)
	{
		if(IsStarting)
		{
		
			SetSound(StarterSound);
			Play();
		}else
		{
			Stop();
			if(Sound!=TempEngineSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(),StarterReleaseSound,GetComponentLocation());
			}
		}
	}
	else{
		
		if(Sound!=TempEngineSound)
		{
			Play();
			UGameplayStatics::PlaySoundAtLocation(GetWorld(),EngineStartSound,GetComponentLocation());
			SetSound(TempEngineSound);
			
		}
		}
	
}
