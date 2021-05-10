// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularVehicleFunctionLibrary.h"

#include "ModularMovementComponent.h"
#include "TitanMovement.h"
#include "Kismet/KismetMathLibrary.h"

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

void UModularVehicleFunctionLibrary::SetThrottleInputOnModularVehicle(APawn* Pawn, float Throttle)
{
	
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetThrottleInput(Throttle);
	}else
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}

void UModularVehicleFunctionLibrary::SetSteerInputOnModularVehicle(APawn* Pawn, float Steer)
{
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetSteeringInput(Steer);
	}else
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}

void UModularVehicleFunctionLibrary::SetHandBrakeInputOnModularVehicle(APawn* Pawn, bool Brake)
{
	if(UModularMovementComponent* MC=Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
	{
		MC->SetHandBrakeInput(Brake);
	}else
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("No Modular MovementComponent Attached to pawn (called from function library)"))
	}
}

FTransform UModularVehicleFunctionLibrary::GetWheelAnimationData(USceneComponent* Wheel)
{
	FTransform Result;
IWheelInterface* WheelInterface=	Cast<IWheelInterface>(Wheel);

	
	if(WheelInterface&&Wheel->GetWorld()->IsGameWorld())
	{
		
	  
		 FWheelState* WheelState=WheelInterface->GetWheelState();

		const FVector WheelRadiusVector=FVector(0,0,WheelState->WheelSetup->WheelRadius);
		const FVector SuspensionTraceLocation=WheelState->HitResult.bBlockingHit?WheelState->HitResult.ImpactPoint+WheelRadiusVector:WheelState->HitResult.TraceEnd;
		//local
		 FVector ContactPointPosition=		Wheel->GetComponentTransform().InverseTransformPosition(SuspensionTraceLocation);
	
		float Sin,Cos;
		
		const float ContactPointAngle=	FMath::Atan2(ContactPointPosition.X,WheelState->WheelSetup->WheelRadius)*2;
		
		FMath::SinCos(&Sin,&Cos,ContactPointAngle);
		
	
	FVector	ResultPosition=FVector::ZeroVector;
	ResultPosition.Z=ContactPointPosition.Z-((FMath::Abs(Sin))*WheelState->WheelSetup->WheelRadius/PI);
	ResultPosition.Y=ContactPointPosition.Y;
	
		
		
	Result.SetLocation(ResultPosition);
	
	Result.SetRotation(UKismetMathLibrary::ComposeRotators(WheelState->InitialLocalRotation,FRotator(FMath::RadiansToDegrees(-1*WheelState->AngularPosition),WheelState->SteerAngle,0)).Quaternion());
	DrawDebugSphere(Wheel->GetWorld(),WheelState->HitResult.ImpactPoint,10,30,FColor::Red,false);
	if(WheelState->SuspAngle!=0.0f)
	{
	const float CurrentAngle=	UKismetMathLibrary::MapRangeClamped(WheelState->HitResult.Time,0,1,0,WheelState->SuspAngle);
		Result.SetRotation((UKismetMathLibrary::ComposeRotators(Result.GetRotation().Rotator(),FRotator(0,0,-CurrentAngle)).Quaternion()));
	}

	}
	return  Result;
}

float UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(UActorComponent* InComponent)
{
	//forms a triangle sum of angles =180
	
	float Result=0;
	if(IWheelInterface* WheelInterface= Cast<IWheelInterface>(InComponent))
	{
		if(WheelInterface->GetWheelState()->WheelSetup->SuspensionPivot!=0)
		{
			const float PivotLen=WheelInterface->GetWheelState()->WheelSetup->SuspensionPivot;
			const float SuspLen=WheelInterface->GetWheelState()->WheelSetup->SuspensionLength;
			const float DeltaX=PivotLen;
			const float DeltaY=SuspLen;
			Result=90-	FMath::RadiansToDegrees(FMath::Atan2(DeltaX,DeltaY));
		}
		
	}
	return  Result;
	
}
