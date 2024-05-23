//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "VehicleAudioComponent.h"

#include "ModularGearBox.h"
#include "ModularMovementComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GenericPlatform/GenericPlatformMath.h"

void UVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();
	
	
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
			if(IsActive())
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
			}else
			{
				
					
						if(MC->VehicleState.IsEngineOn)
						{
							Activate(true);
						}
					
				
			}
		}
	}
}
