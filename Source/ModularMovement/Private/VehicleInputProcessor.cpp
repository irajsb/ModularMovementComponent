// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleInputProcessor.h"

#include "ModularGearBox.h"
#include "ModularMovementComponent.h"

float UVehicleInputProcessor::CalcBrakeInput_Implementation(UModularMovementComponent* MovementComponent,
                                                            float DeltaTime,float RawBrakeInput,float RawThrottleInput)
{

	MovementComponent->IsBraking=false;
	
	auto Setup= MovementComponent->GetSetup();
	float NewBrakeInput =MovementComponent->VehicleState.IsEngineOn? 0.0f:Setup->GetIdleBrakeInput();
	if (Setup->ShouldReverseAsBrake())
	{
		

		// if player wants to move forwards...
		if (RawThrottleInput > 0.f)
		{
			// if vehicle is moving backwards, then press brake
			if (MovementComponent->VehicleState.ForwardSpeed < -Setup->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
				MovementComponent->IsBraking=true;
			}
		}

		// if player wants to move backwards...
		else if (RawThrottleInput < 0.f)
		{
			// if vehicle is moving forwards, then press brake
			if (MovementComponent->VehicleState.ForwardSpeed > Setup->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
				MovementComponent->IsBraking=true;
			}
		}
		// if player isn't pressing forward or backwards...
		else
		{
			if (FMath::Abs(MovementComponent->VehicleState.ForwardSpeed) < Setup->GetStopThreshold())
				
			{
				NewBrakeInput = 1.f;
			}
			else
			{
				NewBrakeInput = Setup->GetIdleBrakeInput();
			
			}
		}

		NewBrakeInput= FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
	}else
	{
		if(RawBrakeInput==0&&RawThrottleInput<0)
		{
			NewBrakeInput=RawThrottleInput;
		}else
		{
			NewBrakeInput = FMath::Abs(RawBrakeInput);
		}
		
	}

	// if player isn't pressing forward or backwards...
	if (FMath::Abs(MovementComponent->RawBrakeInput) < SMALL_NUMBER && FMath::Abs(RawThrottleInput) < SMALL_NUMBER)
	{
		if (MovementComponent->VehicleState.ForwardSpeed < Setup->GetStopThreshold() &&MovementComponent-> VehicleState.ForwardSpeed > -Setup->
			GetStopThreshold()) //auto brake 
		{
			NewBrakeInput = 1.f;
		}
	}


	return NewBrakeInput;
}

float UVehicleInputProcessor::CalcSteerInput_Implementation(UModularMovementComponent* MovementComponent,float DeltaTime,float RawInput)
{
	
	// Determine the rate to use for interpolation
	const float InterpolationSpeed = (RawInput != 0.f || FMath::Sign(RawInput * MovementComponent->SteeringInput) == 1
										  ? MovementComponent->GetSetup()->GetSteerInputRise()
										  : MovementComponent->GetSetup()->GetSteerInputFall());

	// Interpolate between the current steering input and the target
	float Result = FMath::FInterpTo(MovementComponent->SteeringInput, RawInput, DeltaTime, InterpolationSpeed);

	// Clamp the steering input to ensure it's within valid range
	Result = FMath::Clamp(Result, -1.0f, 1.0f);

	return Result;
}

float UVehicleInputProcessor::CalcThrottleInput_Implementation(UModularMovementComponent* MovementComponent,
	float DeltaTime,float RawInput,float RawBrakeInput,float RawSteeringInput)
{

	auto Setup= MovementComponent->GetSetup();
	float NewThrottleInput =RawInput;
	const bool IsInReverse=Setup->GetGearBox()->IsInReverse();
	if (Setup->ShouldReverseAsBrake())
	{
		if (RawBrakeInput > 0.f &&IsInReverse )
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
		{
			//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if (RawInput > 0.f && IsInReverse || RawInput < 0.f&& !IsInReverse)
			{
				NewThrottleInput = 0.f;
			}
		}
	}

	//Throttle and steer are not discrete in a  tank so we calculate both here
	if (Setup->GetSteerType() == Tank)
	{
		NewThrottleInput = FMath::Clamp(FMath::Abs(RawInput) + FMath::Abs(RawSteeringInput), 0.f, 1.f);
	}

	if(!Setup->ShouldReverseAsBrake()&&IsInReverse)
	{
		if(NewThrottleInput>0)
		{
			NewThrottleInput*=-1;
		}else
		{
			if(NewThrottleInput<0)
			{
				NewThrottleInput=0;
			}
		}
	}
	return NewThrottleInput;
}
