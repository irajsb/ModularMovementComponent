//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

/*=============================================================================
	UMMVehicleAnimationInstance.cpp: Single Node Tree Instance 
	Only plays one animation at a time. 
=============================================================================*/ 

#include "MMVehicleAnimationInstance.h"
#include "ModularMovementComponent.h"
#include "AnimationRuntime.h"
#include "ModularVehicleFunctionLibrary.h"
#include "ModularWheel.h"


/////////////////////////////////////////////////////
	// UMMVehicleAnimationInstance
	/////////////////////////////////////////////////////

	UMMVehicleAnimationInstance::UMMVehicleAnimationInstance(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
	}



	void UMMVehicleAnimationInstance::NativeInitializeAnimation()
	{
		Super::NativeInitializeAnimation();
		// Find a wheeled movement component
		if (AActor* Actor = GetOwningActor())
		{
			if (UModularMovementComponent* FoundWheeledVehicleComponent = Actor->FindComponentByClass<UModularMovementComponent>())
			{
				SetWheeledVehicleComponent(FoundWheeledVehicleComponent);
			}
		}
	}

	FAnimInstanceProxy* UMMVehicleAnimationInstance::CreateAnimInstanceProxy()
	{
		return &AnimInstanceProxy;
	}

	void UMMVehicleAnimationInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
	{
	}

	/////////////////////////////////////////////////////
	//// PROXY ///
	/////////////////////////////////////////////////////

	void FMMVehicleAnimationInstanceProxy::SetWheeledVehicleComponent(const UModularMovementComponent* InWheeledVehicleComponent)
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
				
				if(Wheel)
				{
					WheelInstance.BoneName=Wheel->GetFName();
				}
				
			}
		}
	}

	void FMMVehicleAnimationInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
	{
		Super::PreUpdate(InAnimInstance, DeltaSeconds);

		
		
		const UMMVehicleAnimationInstance* VehicleAnimInstance = CastChecked<UMMVehicleAnimationInstance>(InAnimInstance);
		const UModularMovementComponent* ModularMovementComponent=VehicleAnimInstance->GetWheeledVehicleComponent();
		if(!VehicleAnimInstance->GetWheeledVehicleComponent())
		{
			ModularMovementComponent= Cast<UModularMovementComponent>(VehicleAnimInstance->GetOwningActor()->GetComponentByClass(UModularMovementComponent::StaticClass()));
		}
		if ( ModularMovementComponent )
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
				if(Wheel)
				{
					WheelInstance.BoneName=Wheel->GetFName();
				}
			}
		}
	}

