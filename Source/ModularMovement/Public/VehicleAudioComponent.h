//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "VehicleAudioComponent.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UVehicleAudioComponent : public UAudioComponent
{

	virtual void BeginPlay() override;
	GENERATED_BODY()
	UVehicleAudioComponent();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> StarterSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> EngineStartSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> StarterReleaseSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> HandBrakeSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> HandReleaseSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> BrakeStartSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> BrakeLoopSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> BrakeReleaseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TObjectPtr<USoundBase> ReverseSound;
	
	
	
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float RPMInterpolationSpeed=0.5;
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float RPMMultiplier=1;
	
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float LoadInterpolationSpeed=1;
	UPROPERTY(EditAnywhere,Category="EngineSound")
	float LoadMultiplier=1;
	
	UPROPERTY(EditAnywhere,Category="EngineSound")
    float TurboInterpolationSpeed=0.25;
    UPROPERTY(EditAnywhere,Category="EngineSound")
    float TurboMultiplier=1;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void OnEngineStateChange(bool IsEngineOn, bool IsStarting);

	UPROPERTY()
	TObjectPtr<USoundBase> TempEngineSound;

	
	
	UPROPERTY(BlueprintReadOnly,Category="EngineSound")
	float Load;
	UPROPERTY(BlueprintReadOnly,Category="EngineSound")
	float CurrentTurbo;
private:
	float RPM=0.f;
	bool LastHandBrakeInput;
	bool LastBrakeInput;
	bool LastReverseInput;
	UPROPERTY(Transient)
	UAudioComponent * BrakeAudioComponent;
	UPROPERTY(Transient)
	UAudioComponent * ReverseAudioComponent;
};
