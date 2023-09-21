//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularGearBox.h"

#include "Utility/ModuarVehicleDebugger.h"
#include "ModularMovementComponent.h"
#include "ModularWheel.h"
#include "Kismet/KismetMathLibrary.h"


UModularGearBox::UModularGearBox()
{
	//reverse
	Gears.Add(FModularGearInfo(3));
	//idle
	Gears.Add(FModularGearInfo(0));
	//Forward
	Gears.Add(FModularGearInfo(3));
	Gears.Add(FModularGearInfo(2));
	Gears.Add(FModularGearInfo(1.55));
	Gears.Add(FModularGearInfo(1.33));
	Gears.Add(FModularGearInfo(1));
}

void UModularGearBox::SetupGearBox()
{
	for (int Index = 0; Index != Gears.Num(); ++Index)
	{
		if (Gears[Index].GearRatio == 0)
		{
			IdleGear = Index;
			TargetGear=CurrentGear=IdleGear+1;
		}
	}
}
void UModularGearBox::SetTargetGear(int32 GearNum, bool bImmediate,class UModularMovementComponent* MovementComponent)
{
	if (Gears.IsValidIndex(GearNum))
	{
		if (GearChangeTime == 0.f||bImmediate)
		{
			MovementComponent->OnGearChange.Broadcast(CurrentGear, GearNum, true);
			CurrentGear = TargetGear = GearNum;
		}
		else
		{
			TargetGear = GearNum;
			MovementComponent->OnGearChange.Broadcast(CurrentGear, GearNum, false);
			CurrentGearChangeTime =GearChangeTime;
		}
	}
	if (MovementComponent->ModularVehicleDebugger)
	{
		FString Message = ("Setting Target Gear:");
		Message += FString::FromInt(TargetGear) + TEXT(" Current: ") + FString::FromInt(
		CurrentGear);
		if (bImmediate)
		{
			Message += " Immediate shift";
		}
		else
		{
			Message += " Shift With " + FString::SanitizeFloat(GearChangeTime) + " Timer";
		}
		MovementComponent->ModularVehicleDebugger->OnGearboxDebugUpdate.Broadcast(0, Message);
	}
}

void UModularGearBox::Update(float DeltaTime, UModularMovementComponent* MovementComponent)
{
	if(!IsManual)
	{
		FVehicleState VehicleState=MovementComponent->VehicleState;
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
						SetTargetGear(IdleGear - 1, true,MovementComponent);
					}
					if (MovementComponent->RawThrottleInput > KINDA_SMALL_NUMBER && CurrentGear <= IdleGear &&
						TargetGear <= IdleGear)
					{
						SetTargetGear(IdleGear + 1, true,MovementComponent);
					}
					}
				else
				{
					// situations when  car is moving fast and needs to change gear 

					//if car is moving in forward speed and  its in back gear (happens after a -180 degree flip in reverse
					if (VehicleState.ForwardSpeed > KINDA_SMALL_NUMBER &&MovementComponent-> RawThrottleInput > KINDA_SMALL_NUMBER &&
						CurrentGear < IdleGear)
					{
						SetTargetGear(IdleGear + 1, true,MovementComponent);
					}
				}
			}
			float DriveWheelRadius=0.f;
		
			for (const auto Wheel:MovementComponent->GetWheels())
			{
				if(Wheel->WheelState.ApplyDriveForce)
				{
					DriveWheelRadius=Wheel->GetWheelSetup()->WheelRadius/100;
					break;
				}
			}
			//Calculate RPM from vehicle speed instead of wheel because wheel can get locked or spin
			const float CurrentRpm= (VehicleState.ForwardSpeed/100*Gears[CurrentGear].GearRatio*DifferentialRatio/DriveWheelRadius)*30/PI;
			 GearBoxRPMRatio=UKismetMathLibrary::MapRangeClamped(CurrentRpm, MovementComponent->GetSetup()->GetIdleRPM(),
																		   MovementComponent->GetSetup()->GetMaxRPM(), 0, 1);
		
			// not currently changing gear, also don't want to change up because the wheels are spinning up due to having no load
			if (CurrentGear > IdleGear)
			{
				if (CurrentGearChangeTime == 0.f )
				{
					if (GearBoxRPMRatio >= Gears[CurrentGear].UpRatio)
					{
					
						SetTargetGear(CurrentGear + 1, false,MovementComponent);
					}
					else if (GearBoxRPMRatio <= Gears[CurrentGear].DownRatio && CurrentGear > IdleGear + 1) // don't change down to neutral
						{
						SetTargetGear(CurrentGear - 1, true,MovementComponent);
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
}

float UModularGearBox::GetGearRatio()
{
	return Gears[CurrentGear].GearRatio;
}




float UModularGearBox::GetDriveRatio()
{
	return 	Gears[CurrentGear].GearRatio*DifferentialRatio;

	
}


void UModularGearBox::SetCurrentGear(int InGear)
{

	if(Gears.IsValidIndex(InGear))
	{
		CurrentGear=InGear;
	}else
	{
		UE_LOG(LogTemp,Error,TEXT("Invalid gear passed %d"),InGear);
	}
	
}



bool UModularGearBox::IsInReverse()
{
	
		return CurrentGear<IdleGear;

}

bool UModularGearBox::IsChangingGear()
{
	
		return CurrentGearChangeTime>0.f;
	

}

