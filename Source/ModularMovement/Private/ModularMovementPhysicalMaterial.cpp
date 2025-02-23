// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularMovementPhysicalMaterial.h"

float UModularMovementPhysicalMaterial::GetModifiedThrottleForSurface(UModularWheel* Wheel,float Velocity,float MaxSpeedMultiplier)
{

	if (Wheel)
	{
		if (const auto PhysMat= Cast<UModularMovementPhysicalMaterial>( Wheel->GetActivePhysicalMaterial()))
		{
			if (PhysMat->MaxDesiredSpeed*MaxSpeedMultiplier!=0.f&&PhysMat->MaxDesiredSpeed*MaxSpeedMultiplier<FMath::Abs(Velocity))
			{
				return 0.01;
			}
		}
	}
	return 1.f;
}
