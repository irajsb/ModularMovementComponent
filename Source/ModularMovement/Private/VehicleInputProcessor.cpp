// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleInputProcessor.h"

#include "ModularGearBox.h"
#include "ModularMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

float UVehicleInputProcessor::CalcBrakeInput_Implementation(UModularMovementComponent* MovementComponent,
                                                            float DeltaTime, const float RawBrakeInput,
                                                            const float RawThrottleInput)
{
	MovementComponent->IsBraking = MovementComponent->HandBrakeInput;

	const auto Setup = MovementComponent->GetSetup();
	float NewBrakeInput = 0.f;
	
	if (Setup->ShouldReverseAsBrake())
	{
		
		// if player wants to move forwards...
		if (RawThrottleInput > 0.f)
		{
			
			// if vehicle is moving backwards, then press brake
			if (MovementComponent->VehicleState.ForwardSpeed < -Setup->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
				MovementComponent->IsBraking = true;
			}
		}

		// if player wants to move backwards...
		else if (RawThrottleInput < 0.f)
		{
			// if vehicle is moving forwards, then press brake
			if (MovementComponent->VehicleState.ForwardSpeed > Setup->GetWrongDirectionThreshold())
			{
				NewBrakeInput = 1.0f;
				MovementComponent->IsBraking = true;
			}
		}
		
		// if player isn't pressing forward or backwards...
		else
		{
			if (FMath::Abs(MovementComponent->VehicleState.ForwardSpeed) < Setup->GetStopThreshold())

			{
				//NewBrakeInput = 1.f;
			}
			else
			{
				NewBrakeInput = Setup->GetIdleBrakeInput();
			}
		}

		NewBrakeInput = FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
	}
	else
	{
		if (RawBrakeInput == 0 && RawThrottleInput < 0)
		{
			NewBrakeInput = RawThrottleInput;
		}
		else
		{
			NewBrakeInput = FMath::Abs(RawBrakeInput);
		}
	}

	// if player isn't pressing forward or backwards...
	if (FMath::Abs(MovementComponent->RawBrakeInput) < SMALL_NUMBER && FMath::Abs(RawThrottleInput) < SMALL_NUMBER)
	{
		if (MovementComponent->VehicleState.ForwardSpeed < Setup->GetStopThreshold() && MovementComponent->VehicleState.
			ForwardSpeed > -Setup->
			GetStopThreshold()) //auto brake 
		{
			NewBrakeInput = 1.f;
			
		}
	}

	if(MovementComponent->GetSetup()->ParkBrake)
	{
		if (!MovementComponent->bShouldReplicateInput)
			{
				NewBrakeInput=1.f;
			}
		
		
	}
	return NewBrakeInput;
}

float UVehicleInputProcessor::CalcSteerInput_Implementation(UModularMovementComponent* MovementComponent,
                                                            const float DeltaTime, const float RawInput)
{
	// Determine the rate to use for interpolation
	float InterpolationSpeed = RawInput != 0.f || FMath::Sign(RawInput * MovementComponent->SteeringInput) == 1
		                           ? MovementComponent->GetSetup()->GetSteerInputRise()
		                           : MovementComponent->GetSetup()->GetSteerInputFall();

	//Removes speed scale influence from steering speed 
	float SteerSpeedScale = MovementComponent->GetSetup()->GetSteerSpeedScaleForSpeed(
		MovementComponent->VehicleState.ForwardSpeed * 0.036/*CmSToKmH*/);



	
	float Result = RawInput;
	if (IsDrifting(MovementComponent))
	{
		 float OptimalDriftAngle = CalculateOptimalDriftAngle(MovementComponent,
		                                                           MovementComponent->GetMesh()->GetPhysicsLinearVelocity().
		                                                                              Size());
		if(FMath::Sign( MovementComponent->GetMesh()->GetPhysicsAngularVelocityInRadians().Z)!=FMath::Sign(Result))
		{
			OptimalDriftAngle=0.f;
		}
		const float AngleError = OptimalDriftAngle - FMath::Abs(MovementComponent->VehicleState.SlipAngle);

		SteerSpeedScale=SteerSpeedScale/4;
		Result = CalculateSteeringCorrection(AngleError, MovementComponent, Result);
		
	}
	
	// Interpolate between the current steering input and the target
	Result = FMath::FInterpTo(MovementComponent->SteeringInput, Result, DeltaTime,
	                          InterpolationSpeed / SteerSpeedScale);

	// Clamp the steering input to ensure it's within valid range
	Result = FMath::Clamp(Result, -1.0f, 1.0f);


	return Result;
}

float UVehicleInputProcessor::CalcThrottleInput_Implementation(UModularMovementComponent* MovementComponent,
                                                               float DeltaTime, const float RawInput,
                                                               const float RawBrakeInput, const float RawSteeringInput)
{
	const auto Setup = MovementComponent->GetSetup();
	float NewThrottleInput = RawInput;
	const bool IsInReverse = Setup->GetGearBox()->IsInReverse();
	if (Setup->ShouldReverseAsBrake())
	{
		if (RawBrakeInput > 0.f && IsInReverse)
		{
			NewThrottleInput = RawBrakeInput;
		}
		else
		{
			//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
			if ((RawInput > 0.f && IsInReverse) || (RawInput < 0.f && !IsInReverse))
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

	if (!Setup->ShouldReverseAsBrake() && IsInReverse)
	{
		if (NewThrottleInput > 0)
		{
			NewThrottleInput *= -1;
		}
		else
		{
			if (NewThrottleInput < 0)
			{
				NewThrottleInput = 0;
			}
		}
	}

	if (IsDrifting(MovementComponent))
	{
		NewThrottleInput = CalculateThrottleAdjustment(MovementComponent, NewThrottleInput);
	}
	return NewThrottleInput;
}


float UVehicleInputProcessor::CalculateThrottleAdjustment(const UModularMovementComponent* ModularMovement,
                                                          float InThrottle)
{
	// Reduce throttle if traction is too low
	if (ModularMovement->VehicleState.WheelTraction < 0.2f)
	{
		return 0.f;
	}

	float OptimalDriftAngle = CalculateOptimalDriftAngle(ModularMovement,
	                                                     ModularMovement->GetMesh()->GetPhysicsLinearVelocity().Size());
	float AngleError = OptimalDriftAngle - FMath::Abs(ModularMovement->VehicleState.SlipAngle);
	if (AngleError > FMath::DegreesToRadians(ModularMovement->GetSetup()->ThrottleCutoffAngle))
	{
		return 0.f;
	}
	return InThrottle;
}


float UVehicleInputProcessor::CalculateSteeringCorrection(float AngleError,
                                                          const UModularMovementComponent* ModularMovement,
                                                          float InSteer)
{
	InSteer = FMath::Sign( ModularMovement->GetMesh()->GetPhysicsAngularVelocityInRadians().Z  )* UKismetMathLibrary::MapRangeClamped(AngleError, 0.1, -0.1, 1.f, -1.f);

	if (AngleError < 0)
	{
		ModularMovement->GetMesh()->AddTorqueInRadians(
			ModularMovement->GetMesh()->GetUpVector() * AngleError * ModularMovement->GetSetup()->DriftAssistTorque *
			FMath::Sign(ModularMovement->GetMesh()->GetPhysicsAngularVelocityInRadians().Z));
	
	}

	return InSteer;
}


float UVehicleInputProcessor::CalculateOptimalDriftAngle(const UModularMovementComponent* ModularMovement,
                                                         float Velocity)
{
	const auto Setup = ModularMovement->GetSetup();

	// Higher speeds need smaller drift angles for control
	return FMath::DegreesToRadians(UKismetMathLibrary::MapRangeClamped(Velocity, Setup->MinDriftSpeed,
	                                                                   Setup->MaxDriftSpeed, Setup->BaseOptimalAngle,
	                                                                   Setup->MaxSpeedOptimalAngle));
}


bool UVehicleInputProcessor::IsDrifting(const UModularMovementComponent* ModularMovement)
{
	if (ModularMovement->GetSetup()->DriftAssistEnabled && !ModularMovement->IsInReverse())
	{
		if(ModularMovement->GetNumberOfWheelsTouchingGround()!=0)
		{
			if(ModularMovement->GetMesh()->GetPhysicsLinearVelocity().Size() > ModularMovement->GetSetup()->MinDriftSpeed)
				return (FMath::Abs(ModularMovement->VehicleState.SlipAngle) > FMath::DegreesToRadians(
					ModularMovement->GetSetup()->DriftAngleThreshold));
		}
	}
	return false;
}
