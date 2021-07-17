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

void UModularVehicleFunctionLibrary::GetWheelAnimationData(UModularWheel* Wheel,FVector& Location,FRotator& Rotation,float DeltaTime)
{
	



	if(Wheel->GetWorld()->IsGameWorld())
	{
		
	  
		 FWheelState* WheelState=Wheel->GetWheelState();

		const FVector WheelRadiusVector=FVector(0,0,WheelState->WheelSetup->WheelRadius);
		const FVector SuspensionTraceLocation=WheelState->HitResult.bBlockingHit?WheelState->HitResult.ImpactPoint+WheelRadiusVector:WheelState->HitResult.TraceEnd;
		
		
		
		const FTransform WheelTransform=Wheel->GetWheelTransform();
			
		//local
		  FVector ContactPointPosition=		WheelTransform.InverseTransformPosition(SuspensionTraceLocation);
		  ContactPointPosition=	ContactPointPosition=WheelTransform.GetRotation().RotateVector(ContactPointPosition);
		

	
		float Sin,Cos;
		
		const float ContactPointAngle=	FMath::Atan2(ContactPointPosition.X,WheelState->WheelSetup->WheelRadius)*2;
		
		FMath::SinCos(&Sin,&Cos,ContactPointAngle);
		
	
	FVector	ResultPosition=FVector::ZeroVector;
	ResultPosition.Z=ContactPointPosition.Z-((FMath::Abs(Sin))*WheelState->WheelSetup->WheelRadius/PI);
	
		
	
	if(WheelState->SuspAngle!=0.0f)
	{
		const float PivotAngle=	FMath::Atan2(ResultPosition.Z,WheelState->WheelSetup->SuspensionPivot);
		FMath::SinCos(&Sin,&Cos,PivotAngle);
		ResultPosition.Y=FMath::Abs(Sin)*WheelState->WheelSetup->SuspensionPivot* FMath::Sign(WheelState->InitialLocalLocation.Y) *-0.5;
	}

	if(WheelState->WheelSetup->AnimSpeed!=0.0f)
	{
		ResultPosition=UKismetMathLibrary::VInterpTo_Constant(WheelState->PreviousLocation,ResultPosition,DeltaTime,WheelState->WheelSetup->AnimSpeed);
	}
	
		
	
	Location=ResultPosition;
	WheelState->PreviousLocation=ResultPosition;
	
	const float Steer=WheelState->MovementComponent?UKismetMathLibrary::FInterpTo_Constant(WheelState->PreviousYaw, WheelState->SteerAngle,DeltaTime,WheelState->MovementComponent->VehicleState.VehicleData->SteeringAnimationSpeed):WheelState->SteerAngle;
	
		
	Rotation=FRotator(FMath::RadiansToDegrees(-1*WheelState->AngularPosition),Steer,0);

	WheelState->PreviousYaw=Steer;
	
	if(WheelState->SuspAngle!=0.0f)
	{
		
	const float CurrentAngle=	UKismetMathLibrary::MapRangeClamped(ResultPosition.Z,WheelState->WheelSetup->SuspensionLength,-WheelState->WheelSetup->SuspensionLength,-WheelState->SuspAngle,WheelState->SuspAngle);
	Rotation=UKismetMathLibrary::ComposeRotators(Rotation,FRotator(0.00f,0.00f,-CurrentAngle));
	
	WheelState->CurrentPivotAngle	=CurrentAngle;
	
		
	}

	}
	
}

float UModularVehicleFunctionLibrary::GetWheelRotation(UModularWheel* Wheel)
{

	if(Wheel)
	return FMath::RadiansToDegrees(-1*Wheel->GetWheelState()->AngularPosition);
	return 0.0f;
}

float UModularVehicleFunctionLibrary::GetWheelPivotRotation(UModularWheel* Wheel)
{
	
	if(Wheel)
		return Wheel->GetWheelState()->CurrentPivotAngle;
	return 0.0f;
}

float UModularVehicleFunctionLibrary::GetWheelSteeringValue(UModularWheel* Wheel)
{
	
	if(Wheel)
	return Wheel->GetWheelState()->SteerAngle;
	return 0.0f;
}

float UModularVehicleFunctionLibrary::GetWheelCompressionValue(UModularWheel* Wheel)
{	
	if(Wheel)
		return 1-Wheel->GetWheelState()->HitResult.Time;
	return 0.0f;
}

float UModularVehicleFunctionLibrary::GetWheelRPM(UModularWheel* Wheel)
{
	
	if(Wheel)
	return 	Wheel->GetWheelState()->Omega * 30.f / PI;
	return 0.0f;
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
