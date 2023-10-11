//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularVehicleData.h"

#include "ModularGearBox.h"

UModularVehicleData::UModularVehicleData()
{

	
	SuspensionTraceTypeQuery= UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	WrongDirectionThreshold=100.f;
	StopThreshold = 10.0f;
	IdleBrakeInput=0.1f;
	ReverseThreshold=0.4;
	DesireSpeedNormal=200;
	DesireSpeedTurningAround=-40;
	DesireSpeedNearGoal=20;
	NearGoalDistance=5000;
	FullThrottleSpeed=40;
	AITraceLength=50;
	AITraceSpeedMultiplier=0.01;
	TurnThreshold=0.4;
	DesireSpeedTurning=10;
	AIMaxSteerMultiplier=1.2;
	ScaleDriveTorqueToNumberOfWheels=true;

	GearBoxData=CreateDefaultSubobject<UModularGearBox>("DefaultGearBox");

	if(EngineTorqueCurve.GetRichCurve()->IsEmpty())
	{
		auto Curve=EngineTorqueCurve.GetRichCurve();
		auto Key=Curve->AddKey(0,250);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Cubic);
		Key=Curve->AddKey(2000,350);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Cubic);
		Key=Curve->AddKey(5000,150);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Cubic);
		Key=Curve->AddKey(6000,0);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Cubic);
	
		
	}
	if(SteerCurve.GetRichCurve()->IsEmpty())
	{
		auto Curve=SteerCurve.GetRichCurve();
		auto Key=Curve->AddKey(0,1);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Linear);
		Key=Curve->AddKey(1,1);
		Curve->SetKeyInterpMode(Key,ERichCurveInterpMode::RCIM_Linear);
	}
}

float UModularVehicleData::GetIdleRPM() const
{
	return  IdleRpm;
}

float UModularVehicleData::GetMaxRPM() const
{
	return MaxRpm;
}

bool UModularVehicleData::ShouldZeroRpmWhenShifting() const
{
	return  ZeroRpmWhenShifting;
}

float UModularVehicleData::GetTorqueForRPM(float RPM) const
{
	return EngineTorqueCurve.GetRichCurveConst()->Eval(RPM);
}

float UModularVehicleData::GetEngineInertia() const
{
	return EngineInertia;
}

UModularGearBox* UModularVehicleData::GetGearBox()
{
	return GearBoxData;
}

bool UModularVehicleData::GetScaleTireFrictionWithSurfaceAngle()
{
	return ScaleTireFrictionWithSurfaceAngle;
}


float UModularVehicleData::GetAirDragConstant() const
{
	return 0.5*AirDragCoefficient*VehicleFrontArea;
}

bool UModularVehicleData::ShouldScaleDriveTorqueToNumberOfWheels() const
{
	return  ScaleDriveTorqueToNumberOfWheels;
}

bool UModularVehicleData::ShouldReverseAsBrake() const
{
	return bReverseAsBrake;
}

float UModularVehicleData::GetStopThreshold() const
{
	return  StopThreshold;
}

float UModularVehicleData::GetWrongDirectionThreshold()
{
	return WrongDirectionThreshold;
}

TEnumAsByte<ETraceTypeQuery> UModularVehicleData::GetSuspensionTraceTypeQuery() const
{
	return SuspensionTraceTypeQuery;
}

float UModularVehicleData::GetReverseThreshold() const
{
	return  ReverseThreshold;
}

float UModularVehicleData::GetIdleBrakeInput() const
{
	return IdleBrakeInput;
}

float UModularVehicleData::GetSteerSpeedScaleForSpeed(float Speed) 
{
	return SteerCurve.GetRichCurve()->IsEmpty()?1:SteerCurve.GetRichCurve()->Eval(Speed);
}

EModularSteerType UModularVehicleData::GetSteerType() const
{
	return SteerType;
}

float UModularVehicleData::GetSteerInputRise() const
{
	return SteerInputRise;
}

float UModularVehicleData::GetSteerInputFall() const
{
	return SteerInputFall;
}





void UModularVehicleData::GetAckermannValues(float& OutWheelBase, float& OutTrackWidth)
{
	OutWheelBase=WheelBase/100.f;
	OutTrackWidth=TrackWidth/100.f;
}

float UModularVehicleData::GetCounterSteerMultiplier() const
{
	
return CounterSteerMultiplier;
}

float UModularVehicleData::GetAITraceLength() const
{
	return AITraceLength;
}

float UModularVehicleData::GetAITraceSpeedMultiplier() const
{
	return AITraceSpeedMultiplier;
}

float UModularVehicleData::GetNearGoalDistance()
{
	return NearGoalDistance;
}

float UModularVehicleData::GetDesireSpeedNearGoal() const
{
	return DesireSpeedNearGoal;
}

float UModularVehicleData::GetDesireSpeedNormal() const
{
	return DesireSpeedNormal;
}

float UModularVehicleData::GetDesireSpeedTurning() const
{
	return  DesireSpeedTurning;
}

float UModularVehicleData::GetDesireSpeedTurningAround() const
{
	return DesireSpeedTurningAround;
}

float UModularVehicleData::GetFullThrottleSpeed() const
{
	return FullThrottleSpeed;
}

float UModularVehicleData::GetAIMaxSteerMultiplier() const
{
	return AIMaxSteerMultiplier;
}

float UModularVehicleData::GetTurnThreshold() const
{
	return TurnThreshold;
	
}
