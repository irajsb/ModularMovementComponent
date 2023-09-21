//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "Utility/ModuarVehicleDebugger.h"
#include "ModularMovement.h"
#include "ModularMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "Utility/VehicleDebugWidget.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UModularVehicleDebugger::UModularVehicleDebugger(): MovementComponent(nullptr), EngineTorque(0), WheelTorque(0),
                                                    ThrottleInput(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UModularVehicleDebugger::BeginPlay()
{
	Super::BeginPlay();

	// ...
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());


	MovementComponent = Cast<UModularMovementComponent>(OwnerPawn->GetMovementComponent());
	if(MovementComponent)
	{
		if(MovementComponent->GetSetup()&&MovementComponent->GetSetup()->GetGearBox()){
			const AController* Controller = OwnerPawn->GetController();
			if (Controller && Controller->IsLocalPlayerController())
			{
				APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
				//	ConstructorHelpers::FClassFinder<UVehicleDebugWidget> ClassFinder(TEXT("WidgetBlueprint'/ModularMovement/DebugMain.DebugMain'"));
				const FString MyActorBpPath = "/ModularMovement/UMG/WB_Debug.WB_Debug_C";

				if (UVehicleDebugWidget* VehicleDebugWidget = CreateWidget<UVehicleDebugWidget>(
					PC, LoadClass<UVehicleDebugWidget>(GetWorld(), *MyActorBpPath)))
				{
					VehicleDebugWidget->AddToViewport(50);
				}
				else
				{
					UE_LOG(LogModularVehicle, Error, TEXT("Cannot init widget"));
				}
			}
		}
	}
}


// Called every frame
void UModularVehicleDebugger::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	NotifyEngineStatus.Broadcast(EngineTorque, WheelTorque, ThrottleInput);
	if (MovementComponent)
	{
		GearBoxChangeGearAllowedStatus = TEXT("Can Change Gear");
	}
	else
	{
		UE_LOG(LogModularVehicle, Error,
		       TEXT(" Debugger must be attached to a pawn that has modular movement Component"))
		MovementComponent = Cast<UModularMovementComponent>(Cast<APawn>(GetOwner())->GetMovementComponent());
	}
	// ...
}
