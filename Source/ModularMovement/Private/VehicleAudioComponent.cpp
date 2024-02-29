// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleAudioComponent.h"

#include "ModularMovementComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"

UVehicleAudioComponent::UVehicleAudioComponent()
{
	PrimaryComponentTick.bCanEverTick=true;
}

void UVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(auto Pawn=Cast<APawn>(GetOwner()))
	{

		if(auto MC= Cast<UModularMovementComponent>( Pawn->GetMovementComponent()))
		{
			auto RPMRatio=MC->VehicleState.CurrentRpm/MC->VehicleState.VehicleData->GetMaxRPM();
			const float RPMChange=RPMRatio-RPM;
			RPM=UKismetMathLibrary::FInterpTo_Constant(RPM,RPMRatio,DeltaTime,RPMInterpolationSpeed);
			SetFloatParameter("RPM",RPM*RPMMultiplier);

			const float NewLoad=(FMath::Abs(MC->ThrottleInput)/2)+(RPMChange > 0) ? 0.5f : 0.0f;
			Load=UKismetMathLibrary::FInterpTo_Constant(Load,NewLoad,DeltaTime,LoadInterpolationSpeed);
			SetFloatParameter("Load",Load*LoadMultiplier);
		}
	}
}
