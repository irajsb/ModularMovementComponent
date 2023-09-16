//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "ModularVehicleAnimationInstance.generated.h"

class UModularMovementComponent;

struct FModularWheelAnimationData
{
	FName BoneName;
	FRotator RotOffset;
	FVector LocOffset;
};

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct MODULARMOVEMENT_API FModularVehicleAnimationInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FModularVehicleAnimationInstanceProxy()
		: FAnimInstanceProxy()
		  , WheelSpokeCount(0)
		  , MaxAngularVelocity(256.f)
		  , ShutterSpeed(30.f)
		  , StageCoachBlend(730.f)
	{
	}

	FModularVehicleAnimationInstanceProxy(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
		  , WheelSpokeCount(0)
		  , MaxAngularVelocity(256.f)
		  , ShutterSpeed(30.f)
		  , StageCoachBlend(730.f)
	{
	}

public:
	void SetWheeledVehicleComponent(const UModularMovementComponent* InWheeledVehicleComponent);

	/** FAnimInstanceProxy interface begin*/
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	/** FAnimInstanceProxy interface end*/

	const TArray<FModularWheelAnimationData>& GetWheelAnimData() const
	{
		return WheelInstances;
	}

	void SetStageCoachEffectParams(int InWheelSpokeCount, float InMaxAngularVelocity, float InShutterSpeed,
	                               float InStageCoachBlend)
	{
		WheelSpokeCount = InWheelSpokeCount;
		MaxAngularVelocity = InMaxAngularVelocity;
		ShutterSpeed = InShutterSpeed;
		StageCoachBlend = InStageCoachBlend;
	}

private:
	TArray<FModularWheelAnimationData> WheelInstances;

	int WheelSpokeCount; // Number of spokes visible on wheel
	float MaxAngularVelocity; // Wheel max rotation speed in degrees/second
	float ShutterSpeed; // Camera shutter speed in frames/second
	float StageCoachBlend; // Blend effect degrees/second
};

UCLASS(transient)
class MODULARMOVEMENT_API UModularVehicleAnimationInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()


public:


public:
	void SetWheeledVehicleComponent(const UModularMovementComponent* InWheeledVehicleComponent)
	{
		WheeledVehicleComponent = InWheeledVehicleComponent;
		AnimInstanceProxy.SetWheeledVehicleComponent(InWheeledVehicleComponent);
	}

	const UModularMovementComponent* GetWheeledVehicleComponent() const
	{
		return WheeledVehicleComponent;
	}

private:
	/** UAnimInstance interface begin*/
	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
	/** UAnimInstance interface end*/

	FModularVehicleAnimationInstanceProxy AnimInstanceProxy;

	UPROPERTY(transient)
	TObjectPtr<const UModularMovementComponent> WheeledVehicleComponent;
};
