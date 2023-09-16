//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "TrackableComponent.h"

// Sets default values for this component's properties
UTrackableComponent::UTrackableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	ContactPoints.Add(FVector2D(0,-1));
	// ...
}


// Called when the game starts
void UTrackableComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTrackableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

