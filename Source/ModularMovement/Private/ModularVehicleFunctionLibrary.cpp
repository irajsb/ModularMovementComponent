// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularVehicleFunctionLibrary.h"

#include "ModularMovementComponent.h"
#include "ModularMovement.h"
#include "WidgetComponent.h"
#include  "ModularWheel.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/PhysicsSettings.h"

float UModularVehicleFunctionLibrary::GetEngineRpm(UModularMovementComponent* MovementComponent)
{
return 	MovementComponent->VehicleState.CurrentRpm;
}

float UModularVehicleFunctionLibrary::GetEngineRpmRatio(UModularMovementComponent* MovementComponent)
{
	return 	MovementComponent->VehicleState.CurrentRpmRatio;
}

int UModularVehicleFunctionLibrary::GetCurrentGear(UModularMovementComponent* MovementComponent)
{
	return 	MovementComponent->VehicleState.CurrentGear;
}

int UModularVehicleFunctionLibrary::GetIdleGear(UModularMovementComponent* MovementComponent)
{
	return  MovementComponent->VehicleState.IdleGear;
}

float UModularVehicleFunctionLibrary::GetCurrentGearRatio(UModularMovementComponent* MovementComponent)
{
return	MovementComponent->VehicleState.VehicleData->Gears[MovementComponent->VehicleState.CurrentGear].GearRatio;
}

float UModularVehicleFunctionLibrary::GetGearRatio(UModularMovementComponent* MovementComponent, int Index,bool& ValidIndex)
{
	ValidIndex=MovementComponent->VehicleState.VehicleData->Gears.IsValidIndex(Index);
	if(ValidIndex)
	return	MovementComponent->VehicleState.VehicleData->Gears[Index].GearRatio;
	return 0.0f;
}

bool UModularVehicleFunctionLibrary::IsChangingGear(UModularMovementComponent* MovementComponent)
{
	if(MovementComponent)
	{
		if(MovementComponent->VehicleState.CurrentGear!=MovementComponent->VehicleState.TargetGear)
			return  true;
	}
	return false;
}


void UModularVehicleFunctionLibrary::SetThrottleInputOnModularVehicle(APawn* Pawn, float Throttle)
{
	if(Pawn)
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetThrottleInput(Throttle);
	}else
	{
		UE_LOG(LogModularVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}

void UModularVehicleFunctionLibrary::SetSteerInputOnModularVehicle(APawn* Pawn, float Steer)
{
	if(Pawn)
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetSteeringInput(Steer);
	}else
	{
		UE_LOG(LogModularVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}

void UModularVehicleFunctionLibrary::SetHandBrakeInputOnModularVehicle(APawn* Pawn, bool Brake)
{
	if(Pawn)
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetHandBrakeInput(Brake);
	}else
	{
		UE_LOG(LogModularVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}


int UModularVehicleFunctionLibrary::GetForwardSpeedKMH(UModularMovementComponent* MovementComponent)
{
	return  static_cast<int>(MovementComponent->VehicleState.ForwardSpeed * 0.036);
}

int UModularVehicleFunctionLibrary::GetForwardSpeedMPH(UModularMovementComponent* MovementComponent)
{
	return  static_cast<int>(MovementComponent->VehicleState.ForwardSpeed * 0.0223694);
}

float UModularVehicleFunctionLibrary::GetForwardSpeedCMs(UModularMovementComponent* MovementComponent)
{
	return MovementComponent->VehicleState.ForwardSpeed;
}

int UModularVehicleFunctionLibrary::GetWheelsTouchingGround(UModularMovementComponent* MovementComponent)
{
	int WheelCount=0;
	for(UModularWheel* Wheel : MovementComponent->Components)
	{
		if(Wheel->WheelState.HitResult.bBlockingHit)
		{
			WheelCount++;
		}
	}
	return  WheelCount;
}


float UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(UModularWheel* Wheel)
{
	//forms a triangle sum of angles =180
	if(!Wheel->GetWheelSetup())
	{
		return 0.0f;
	}
	float Result=0;
	
		if(Wheel->GetWheelState()->WheelSetup->SuspensionPivot!=0)
		{
			const float PivotLen=Wheel->GetWheelState()->WheelSetup->SuspensionPivot;
			const float SuspLen=Wheel->GetWheelState()->WheelSetup->SuspensionLength;
			const float DeltaX=PivotLen;
			const float DeltaY=SuspLen;
			Result=90-	FMath::RadiansToDegrees(FMath::Atan2(DeltaX,DeltaY));
		}
		
	
	return  Result;
	
}

bool UModularVehicleFunctionLibrary::CapsuleTraceSingleWithRotation(UObject* WorldContextObject, const FVector Start, const FVector End,FRotator Rot, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);

	static const FName CapsuleTraceSingleName(TEXT("CapsuleTraceSingle"));
	FCollisionQueryParams Params =ConfigureCollisionParams(CapsuleTraceSingleName, bTraceComplex, ActorsToIgnore, bIgnoreSelf, WorldContextObject);

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	bool const bHit = World ? World->SweepSingleByChannel(OutHit, Start, End, Rot.Quaternion(), CollisionChannel, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params) : false;

#if ENABLE_DRAW_DEBUG
	//DrawDebugCapsuleTraceSingle(World, Start, End, Radius, HalfHeight, DrawDebugType, bHit, OutHit, TraceColor, TraceHitColor, DrawTime);
#endif

	return bHit;
}

FCollisionQueryParams UModularVehicleFunctionLibrary::ConfigureCollisionParams(FName TraceTag, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, bool bIgnoreSelf, UObject* WorldContextObject)
{FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(KismetTraceUtils), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable; // Ask for face index, as long as we didn't disable globally
	Params.AddIgnoredActors(ActorsToIgnore);
	if (bIgnoreSelf)
	{
		AActor* IgnoreActor = Cast<AActor>(WorldContextObject);
		if (IgnoreActor)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		else
		{
			// find owner
			UObject* CurrentObject = WorldContextObject;
			while (CurrentObject)
			{
				CurrentObject = CurrentObject->GetOuter();
				IgnoreActor = Cast<AActor>(CurrentObject);
				if (IgnoreActor)
				{
					Params.AddIgnoredActor(IgnoreActor);
					break;
				}
			}
		}
	}

	return Params;
}

void UModularVehicleFunctionLibrary::UpdateWheelState(UModularWheel* Wheel, FWheelState NewWheelState)
{
	Wheel->UpdateWheelState(NewWheelState);
}



void UModularVehicleFunctionLibrary::GetDebugData(UModularWheel* Wheel, float& LateralFrictionRatio,
                                                  float& LongitudinalFrictionRatio)
{
	
	if(Wheel)
	{
		LongitudinalFrictionRatio= Wheel->GetWheelState()->LongitudinalFrictionRatio;
		LateralFrictionRatio= Wheel->GetWheelState()->LateralFrictionRatio;
	}
}
