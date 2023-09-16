//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

/*=============================================================================
	UModularVehicleAnimationInstance.cpp: Single Node Tree Instance 
	Only plays one animation at a time. 
=============================================================================*/ 

#include "ModularVehicleAnimationInstance.h"
#include "ModularMovementComponent.h"
#include "AnimationRuntime.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularWheel.h"


/////////////////////////////////////////////////////
	// UModularVehicleAnimationInstance
	/////////////////////////////////////////////////////

	UModularVehicleAnimationInstance::UModularVehicleAnimationInstance(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
	}



	void UModularVehicleAnimationInstance::NativeInitializeAnimation()
	{
		// Find a wheeled movement component
		if (AActor* Actor = GetOwningActor())
		{
			if (UModularMovementComponent* FoundWheeledVehicleComponent = Actor->FindComponentByClass<UModularMovementComponent>())
			{
				SetWheeledVehicleComponent(FoundWheeledVehicleComponent);
			}
		}
	}

	FAnimInstanceProxy* UModularVehicleAnimationInstance::CreateAnimInstanceProxy()
	{
		return &AnimInstanceProxy;
	}

	void UModularVehicleAnimationInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
	{
	}

	/////////////////////////////////////////////////////
	//// PROXY ///
	/////////////////////////////////////////////////////

	void FModularVehicleAnimationInstanceProxy::SetWheeledVehicleComponent(const UModularMovementComponent* InWheeledVehicleComponent)
	{
		const UModularMovementComponent* WheeledVehicleComponent = InWheeledVehicleComponent;

		//initialize wheel data
		const int32 NumOfwheels = WheeledVehicleComponent->GetNumberOfWheels();
		WheelInstances.Empty(NumOfwheels);
		if (NumOfwheels > 0)
		{
			WheelInstances.AddZeroed(NumOfwheels);

			for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
			{
				FModularWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
				UModularWheel* Wheel = WheeledVehicleComponent->Components[WheelIndex];
				
				
				WheelInstance.BoneName=Wheel->GetFName();
				
			}
		}
	}

	void FModularVehicleAnimationInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
	{
		Super::PreUpdate(InAnimInstance, DeltaSeconds);

		const UModularVehicleAnimationInstance* VehicleAnimInstance = CastChecked<UModularVehicleAnimationInstance>(InAnimInstance);
		if (const UModularMovementComponent* ModularMovementComponent = VehicleAnimInstance->GetWheeledVehicleComponent())
		{
			if(ModularMovementComponent->Components.Num()!=WheelInstances.Num())
			{
				SetWheeledVehicleComponent(ModularMovementComponent);
			}
			
			for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
			{
				FModularWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
				UModularWheel* Wheel = ModularMovementComponent->Components[WheelIndex];

				FVector Location;
				FRotator Rotation;
				UModularVehicleFunctionLibrary::	GetWheelAnimationData(Wheel,Location, Rotation,DeltaSeconds);
				WheelInstance.LocOffset=Location;
				WheelInstance.RotOffset=Rotation;
				WheelInstance.BoneName=Wheel->GetFName();
			}
		}
	}

