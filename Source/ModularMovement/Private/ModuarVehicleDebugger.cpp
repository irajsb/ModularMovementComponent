// Fill out your copyright notice in the Description page of Project Settings.


#include "ModuarVehicleDebugger.h"
#include "Engine.h"
#include "ModularMovement.h"
#include "ModularMovementComponent.h"
#include "UserWidget.h"
#include "VehicleDebugWidget.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UModuarVehicleDebugger::UModuarVehicleDebugger()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UModuarVehicleDebugger::BeginPlay()
{
	Super::BeginPlay();

	// ...
	MovementComponent=Cast<UModularMovementComponent>(Cast<APawn>(GetOwner())->GetMovementComponent());
	if(Cast<APawn>(GetOwner())->GetController()->IsLocalPlayerController())
	{
		APlayerController* PC= Cast<APlayerController>(	Cast<APawn>(GetOwner())->GetController());
	//	ConstructorHelpers::FClassFinder<UVehicleDebugWidget> ClassFinder(TEXT("WidgetBlueprint'/ModularMovement/DebugMain.DebugMain'"));
		const FString MyActorBpPath = "/ModularMovement/DebugMain.DebugMain_C";
		
		UVehicleDebugWidget* VehicleDebugWidget=	CreateWidget<UVehicleDebugWidget>(PC,LoadClass<UVehicleDebugWidget>(GetWorld(),*MyActorBpPath));
		
		if(VehicleDebugWidget)
		{
			VehicleDebugWidget->AddToViewport(50);
			
			
		}else
		{
			UE_LOG(LogModularVehicle,Error,TEXT("Cannot init widget"));
		}
	}
	
}


// Called every frame
void UModuarVehicleDebugger::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(MovementComponent)
	{


		
	}else
	{
		UE_LOG(LogModularVehicle,Error,TEXT(" Debugger must be attached to a pawn that has modular movement Component"))
		MovementComponent=Cast<UModularMovementComponent>(Cast<APawn>(GetOwner())->GetMovementComponent());
	}
	// ...
}


