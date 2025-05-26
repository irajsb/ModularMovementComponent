// Copyright notice and project settings
//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ModularGearBox.generated.h"

// Structure definition for gear information
USTRUCT(BlueprintType)
struct FModularGearInfo
{
	GENERATED_USTRUCT_BODY()

	// The ratio for this specific gear
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=Transmission)
	float GearRatio;

	// Lower limit for RPM, under which the car should gear down
	UPROPERTY(EditAnywhere,Category=Transmission, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DownRatio;

	// Upper limit for RPM, over which the car should gear up
	UPROPERTY(EditAnywhere,Category=Transmission, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float UpRatio;

	// Default constructor with default values
	FModularGearInfo(): GearRatio(0.8), DownRatio(0.2), UpRatio(1.f)
	{
	};

	// Constructor allowing gear ratio to be set
	FModularGearInfo(float Ratio): GearRatio(Ratio), DownRatio(0.2f), UpRatio(0.8f)
	{
	}
};

// Class definition for the gearbox
UCLASS(EditInlineNew)
class MODULARMOVEMENT_API UModularGearBox : public UObject
{
public:
	GENERATED_BODY()

	void SetupGearBox();
	
	// Constructor
	UModularGearBox();

	// Efficiency of the transmission (0 to 1, where 1 is 100% efficient)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transmission,
		Meta=( UIMin="0", UIMax="1", ClampMin="0.0", ClampMax="1.0"))
	float TransmissionEfficiency = 1;

	//Is gearbox Manual
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transmission)
	bool IsManual;

	//Desired ideal rpm for this vehicle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transmission)
	float IdealRPMRatio=0.3;
	

	// Time required to change gears
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transmission)
	float GearChangeTime = 0.5;

	// Array to store information about all gears
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Transmission)
	TArray<FModularGearInfo> Gears;

	// The idle gear (usually 0 or 'N')
	UPROPERTY(BlueprintReadOnly,Category=Transmission)
	int IdleGear;

	// The current gear the vehicle is in
	UPROPERTY(BlueprintReadOnly,Transient,Category=Transmission)
	int CurrentGear;

	// The gear the vehicle is aiming to shift into
	UPROPERTY(BlueprintReadOnly,Transient,Category=Transmission)
	int TargetGear;
	
	// Time counter for gear change
	UPROPERTY(BlueprintReadOnly, Transient,Category=Transmission)
	float CurrentGearChangeTime=0.f;

	// The current gear the vehicle is in
	UPROPERTY(BlueprintReadOnly,Transient,Category=Transmission)
	float GearBoxRPMRatio=0.f;
	// Sets the target gear, either immediately or not, depending on 'bImmediate'
	UFUNCTION(BlueprintCallable,Category=Transmission)
	void SetTargetGear(int32 GearNum, bool bImmediate, class UModularMovementComponent* MovementComponent);
	void CalculateIdealGear(float IdealGearRatio, int& ClosestGearIndex,int DefaultGear);

	// Update function to be called every frame or tick
	virtual void Update(float DeltaTime, class UModularMovementComponent* MovementComponent);

	// Function to get the ratio of the current gear
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Transmission)
	float GetGearRatio();

	// Get final drive ratio ( Gear* Differential ) 
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Transmission)
	virtual  float GetDriveRatio();

	

	//Set target gear
	UFUNCTION(BlueprintCallable,Category=Transmission)
	virtual  void  SetCurrentGear(int InGear);
	//Set target gear
	UFUNCTION(BlueprintCallable,Category=Transmission)
	virtual  void  SetToIdle();

	//Is current gear not equal to target gear
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Transmission)
	virtual bool IsChangingGear();
	// Is current gear less than idle
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Transmission)
	virtual bool IsInReverse();

	
	UFUNCTION(BlueprintCallable,BlueprintPure,Category=Transmission)
	virtual bool IsIdle();
	
	UPROPERTY(Transient)
	UModularMovementComponent* MC;


	UPROPERTY(Transient)
	float CurrentRpm=0.f;

	int32 MaxGear=9999;
	//
	UFUNCTION(BlueprintCallable,Category=Transmission)
	void SetMaxGear(int32 InGear);
	UFUNCTION(BlueprintCallable,Category=Transmission)
	void ResetMaxGear()
	{
		MaxGear=9999;
	}
};
