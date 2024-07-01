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

	TempEngineSound = Sound;
	if (const auto Pawn = Cast<APawn>(GetOwner()))
	{
		if (const auto MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
		{
			MC->OnEngineStateChange.AddDynamic(
				this, &UVehicleAudioComponent::UVehicleAudioComponent::OnEngineStateChange);
		}
	}
}

UVehicleAudioComponent::UVehicleAudioComponent(): Load(0), CurrentTurbo(0), LastHandBrakeInput(false),
                                                  LastBrakeInput(false),
                                                  LastReverseInput(false),
                                                  BrakeAudioComponent(nullptr),
                                                  ReverseAudioComponent(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = false;
}

void UVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}

	UModularMovementComponent* MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent());
	if (!MC)
	{
		return;
	}
	LastBrakePlayTime-=DeltaTime;
	const float RPMRatio = MC->VehicleState.CurrentRpm / MC->VehicleState.VehicleData->GetMaxRPM();
	const float RPMChange = RPMRatio - RPM;
	const bool GearChanging = MC->GetSetup()->GetGearBox()->IsChangingGear();
	RPM = UKismetMathLibrary::FInterpTo_Constant(RPM, RPMRatio, DeltaTime, RPMInterpolationSpeed);
	SetFloatParameter("RPM", RPM * RPMMultiplier);

	const float NewLoad = GearChanging ? 0.f : ((FMath::Abs(MC->ThrottleInput) / 2) + (RPMChange > 0.05)) ? 0.5f : 0.0f;
	Load = UKismetMathLibrary::FInterpTo_Constant(Load, NewLoad, DeltaTime, LoadInterpolationSpeed);
	SetFloatParameter("Load", Load * LoadMultiplier);

	CurrentTurbo = UKismetMathLibrary::FInterpTo_Constant(CurrentTurbo, RPMRatio, DeltaTime, TurboInterpolationSpeed);
	SetFloatParameter("Turbo", CurrentTurbo * TurboMultiplier);

	if (LastHandBrakeInput != MC->HandBrakeInput)
	{
		LastHandBrakeInput = MC->HandBrakeInput;
		if (LastHandBrakeInput)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HandBrakeSound, GetComponentLocation());
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HandReleaseSound, GetComponentLocation());
		}
	}

	const bool BrakeInput = MC->IsBraking;

	if (BrakeInput != LastBrakeInput)
	{
		LastBrakeInput = BrakeInput;
		if (BrakeInput)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), BrakeStartSound, GetComponentLocation());
			if (BrakeLoopSound)
			{
				if (BrakeAudioComponent)
				{
					BrakeAudioComponent->FadeIn(0.2, 1);
				}
				else
				{
					UActorComponent* Comp = GetOwner()->AddComponentByClass(
						UAudioComponent::StaticClass(), false, FTransform(), false);
					BrakeAudioComponent = Cast<UAudioComponent>(Comp);
					if (BrakeAudioComponent)
					{
						BrakeAudioComponent->SetSound(BrakeLoopSound);
						BrakeAudioComponent->FadeIn(0.2, 1);
					}
				}
			}
		}
		else
		{
			if (BrakeAudioComponent)
			{
				BrakeAudioComponent->FadeOut(0.2, 0);
			}
			if(LastBrakePlayTime>0)
			{
				LastBrakePlayTime=0.2;
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), BrakeReleaseSound, GetComponentLocation());
			}
		}
	}

	if (MC->GetSetup())
	{
		const bool Reverse = MC->GetSetup()->GetGearBox()->IsInReverse();
		if (Reverse != LastReverseInput && ReverseSound)
		{
			LastReverseInput = Reverse;
			if (Reverse)
			{
				if (ReverseAudioComponent)
				{
					ReverseAudioComponent->Play();
				}
				else
				{
					UActorComponent* Comp = GetOwner()->AddComponentByClass(
						UAudioComponent::StaticClass(), false, FTransform(), false);
					ReverseAudioComponent = Cast<UAudioComponent>(Comp);
					if (ReverseAudioComponent)
					{
						ReverseAudioComponent->SetSound(ReverseSound);
						ReverseAudioComponent->Play();
					}
				}
			}
			else
			{
				if (ReverseAudioComponent)
				{
					ReverseAudioComponent->Stop();
				}
			}
		}
	}
}

void UVehicleAudioComponent::OnEngineStateChange(bool IsEngineOn, bool IsStarting)
{
	if (!IsEngineOn)
	{
		if (IsStarting)
		{
			SetSound(StarterSound);
			Play();
		}
		else
		{
			Stop();
			if (Sound != TempEngineSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), StarterReleaseSound, GetComponentLocation());
			}
		}
	}
	else
	{
		if (Sound != TempEngineSound)
		{
			Play();
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), EngineStartSound, GetComponentLocation());
			SetSound(TempEngineSound);
		}
	}
}
