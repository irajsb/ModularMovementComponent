// Fill out your copyright notice in the Description page of Project Settings.


#include "SemiTruckGearBox.h"

#include "ModularMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ModularWheel.h"
void USemiTruckGearBox::Update(float DeltaTime, UModularMovementComponent* MovementComponent)
{
	CurrentCooldown-=DeltaTime;
	
	MC = MovementComponent;
	if (!IsManual)
	{
		const auto  VehicleState = MovementComponent->VehicleState;
		if (VehicleState.DriveWheelsOnGround != 0)
		{
			if (MovementComponent->GetSetup()->ShouldReverseAsBrake())
			{
				//for reverse as state we want to automatically shift between reverse and first gear
				if (FMath::Abs(VehicleState.ForwardSpeed) < MovementComponent->GetSetup()->GetWrongDirectionThreshold())
				//we only shift between reverse and first if the car is slow enough.
				{
					if (MovementComponent->RawThrottleInput < -1 * KINDA_SMALL_NUMBER && CurrentGear >= IdleGear &&
						TargetGear >= IdleGear)
					{
						SetTargetGear(IdleGear - 1, true, MovementComponent);
					}
					if (MovementComponent->RawThrottleInput > KINDA_SMALL_NUMBER && CurrentGear <= IdleGear &&
						TargetGear <= IdleGear)
					{
						SetTargetGear(IdleGear + 1, true, MovementComponent);
					}
				}
				else
				{
					// situations when  car is moving fast and needs to change gear 

					//if car is moving in forward speed and  its in back gear (happens after a -180 degree flip in reverse
					if (VehicleState.ForwardSpeed > KINDA_SMALL_NUMBER && MovementComponent->RawThrottleInput >
						KINDA_SMALL_NUMBER &&
						CurrentGear < IdleGear)
					{
						SetTargetGear(IdleGear + 1, true, MovementComponent);
					}
				}
			}
			float DriveWheelRadius = 0.f;

			for (const auto Wheel : MovementComponent->GetWheels())
			{
				if (Wheel->WheelState.ApplyDriveForce)
				{
					DriveWheelRadius = Wheel->GetWheelSetup()->WheelRadius / 100;
					break;
				}
			}
			//Calculate RPM from vehicle speed instead of wheel because wheel can get locked or spin
			const float CurrentRpm = VehicleState.ForwardSpeed / 100 * Gears[CurrentGear].GearRatio * MovementComponent
				->CurrentDifferentialRatio / DriveWheelRadius * 30 / PI;

			GearBoxRPMRatio = UKismetMathLibrary::MapRangeClamped(
				CurrentRpm, MovementComponent->GetSetup()->GetIdleRPM(),
				MovementComponent->GetSetup()->GetMaxRPM(), 0, 1);
			const float TargetRpm = UKismetMathLibrary::MapRangeUnclamped(
				IdealRPMRatio, 0, 1, MovementComponent->GetSetup()->GetIdleRPM(),
				MovementComponent->GetSetup()->GetMaxRPM());
			const float IdealGearRatio = TargetRpm * PI * DriveWheelRadius / (VehicleState.ForwardSpeed / 100 *
				MovementComponent->CurrentDifferentialRatio * 30);

			// not currently changing gear, also don't want to change up because the wheels are spinning up due to having no load
			if (CurrentGear > IdleGear)
			{
				if (CurrentGearChangeTime == 0.f)
				{

					int ClosestGearIndex;
					CalculateIdealGear(IdealGearRatio, ClosestGearIndex,CurrentGear );
					if(CurrentGear!=ClosestGearIndex)
					{
						
							
						
						if(CurrentCooldown<0.f)
						{	CurrentCooldown=Cooldown;
							SetTargetGear(ClosestGearIndex,false,MovementComponent);
						}
					}
				}
			}
		}
	}

	if (CurrentGear != TargetGear)
	{
		CurrentGearChangeTime -= DeltaTime;
		if (CurrentGearChangeTime <= 0.f)
		{
			CurrentGearChangeTime = 0.f;
			MovementComponent->OnGearChange.Broadcast(CurrentGear, TargetGear, true);
			CurrentGear = TargetGear;
		}
	}
	else
	{
		//Handle cases where immediate shift happens 
		CurrentGearChangeTime = 0.f;
	}
}
