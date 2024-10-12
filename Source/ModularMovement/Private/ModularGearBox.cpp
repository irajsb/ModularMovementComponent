//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularGearBox.h"

#include "Utility/ModuarVehicleDebugger.h"
#include "ModularMovementComponent.h"
#include "ModularWheel.h"
#include "Kismet/KismetMathLibrary.h"


UModularGearBox::UModularGearBox(): IsManual(false), IdleGear(0), CurrentGear(0), TargetGear(0),
                                    MC(nullptr)
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
			TargetGear = CurrentGear = IdleGear + 1;
		}
	}
}

void UModularGearBox::SetTargetGear(int32 GearNum, bool bImmediate, class UModularMovementComponent* MovementComponent)
{
	
		if(GearNum>MaxGear)
		{
			return;
		}
	
	if (Gears.IsValidIndex(GearNum))
	{
		if (GearChangeTime == 0.f || bImmediate)
		{
			MovementComponent->OnGearChange.Broadcast(CurrentGear, GearNum, true);
			CurrentGear = TargetGear = GearNum;
		}
		else
		{
			TargetGear = GearNum;
			MovementComponent->OnGearChange.Broadcast(CurrentGear, GearNum, false);
			CurrentGearChangeTime = GearChangeTime;
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

void UModularGearBox::CalculateIdealGear(const float IdealGearRatio, int& ClosestGearIndex,int DefaultGear)
{
	float ClosestGear = BIG_NUMBER;
	ClosestGearIndex = DefaultGear;
	bool DownShift=DefaultGear<CurrentGear;
	
	for (int Index = IdleGear+1; Index != Gears.Num(); Index++)
	{
		if(!DownShift)
		{
			if (Gears[Index].GearRatio <IdealGearRatio&& IdealGearRatio-Gears[Index].GearRatio   < ClosestGear)
			{
			
				ClosestGear =IdealGearRatio-Gears[Index].GearRatio ;
				ClosestGearIndex = Index;
			
			}
		}else
		{
			if (Gears[Index].GearRatio >IdealGearRatio&&FMath::Abs( IdealGearRatio-Gears[Index].GearRatio )  < ClosestGear)
			{
			
				ClosestGear =FMath::Abs(IdealGearRatio-Gears[Index].GearRatio) ;
				ClosestGearIndex = Index;
			
			}
		}
	}
	
}

void UModularGearBox::Update(float DeltaTime, UModularMovementComponent* MovementComponent)
{
	MC = MovementComponent;
	if (!IsManual)
	{
		FModularVehicleState VehicleState = MovementComponent->VehicleState;
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
			const float CurrentRpm = (VehicleState.ForwardSpeed / 100 * Gears[CurrentGear].GearRatio * MovementComponent
				->CurrentDifferentialRatio / DriveWheelRadius) * 30 / PI;
			
			GearBoxRPMRatio = UKismetMathLibrary::MapRangeClamped(
				CurrentRpm, MovementComponent->GetSetup()->GetIdleRPM(),
				MovementComponent->GetSetup()->GetMaxRPM(), 0, 1);
			const float TargetRpm = UKismetMathLibrary::MapRangeUnclamped(
				IdealRPMRatio, 0, 1, MovementComponent->GetSetup()->GetIdleRPM(),
				MovementComponent->GetSetup()->GetMaxRPM());
			const float IdealGearRatio = (TargetRpm * PI * DriveWheelRadius) / (VehicleState.ForwardSpeed / 100 *
				MovementComponent->CurrentDifferentialRatio * 30);

			if(CurrentGear>MaxGear)
			{
				SetTargetGear(MaxGear, true, MovementComponent);
				return;
			}
			// not currently changing gear, also don't want to change up because the wheels are spinning up due to having no load
			if (CurrentGear > IdleGear)
			{
				if (CurrentGearChangeTime == 0.f)
				{
					if (GearBoxRPMRatio >= Gears[CurrentGear].UpRatio)
					{
						if(Gears.IsValidIndex(CurrentGear + 1))
						{
						
							
							
								SetTargetGear(CurrentGear + 1, false, MovementComponent);
							
						}
					}
					else if ( GearBoxRPMRatio <= Gears[CurrentGear].DownRatio && CurrentGear > IdleGear + 1)
					// don't change down to neutral
					{
						
							SetTargetGear(CurrentGear - 1, true, MovementComponent);
						
						
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

float UModularGearBox::GetGearRatio()
{
	return Gears[CurrentGear].GearRatio;
}


float UModularGearBox::GetDriveRatio()
{
	if (MC)
	{
		return Gears[CurrentGear].GearRatio * MC->CurrentDifferentialRatio;
	}
	return Gears[CurrentGear].GearRatio;
}


void UModularGearBox::SetCurrentGear(int InGear)
{
	if (Gears.IsValidIndex(InGear))
	{
		TargetGear = CurrentGear = InGear;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid gear passed %d"), InGear);
	}
}

void UModularGearBox::SetToIdle()
{
	CurrentGear=TargetGear=IdleGear;
	CurrentGearChangeTime=0.f;
}


bool UModularGearBox::IsInReverse()
{
	return CurrentGear < IdleGear;
}

void UModularGearBox::SetMaxGear(int32 InGear)
{
	MaxGear=InGear;
}

bool UModularGearBox::IsChangingGear()
{
	return CurrentGearChangeTime > 0.f;
}
