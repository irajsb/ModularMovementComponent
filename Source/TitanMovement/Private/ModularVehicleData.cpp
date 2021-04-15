// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularVehicleData.h"

UModularVehicleData::UModularVehicleData()
{
	//reverse
	Gears.Add(FArcadeGearInfo(3));
	//idle
	Gears.Add(FArcadeGearInfo(0));
	//Forward
	Gears.Add(FArcadeGearInfo(3));
	Gears.Add(FArcadeGearInfo(2));
	Gears.Add(FArcadeGearInfo(1.55));
	Gears.Add(FArcadeGearInfo(1.33));
	Gears.Add(FArcadeGearInfo(1));
	
	SuspensionTraceTypeQuery= UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	WrongDirectionThreshold=100.f;
	StopThreshold = 10.0f;
	IdleBrakeInput=0.0f;
}
