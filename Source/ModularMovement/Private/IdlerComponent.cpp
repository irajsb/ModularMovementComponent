//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "IdlerComponent.h"

UIdlerComponent::UIdlerComponent()
{
	Radius=10.f;
	ContactPoints.Reset();
	FContactPoint ContactPoint;
	ContactPoint.ContactPoint=FVector2D(0,1);
	ContactPoints.Add(ContactPoint);
	PrimaryComponentTick.bCanEverTick=false;
}

void UIdlerComponent::BeginPlay()
{
	Super::BeginPlay();
	OriginalLocation=GetRelativeLocation();
}



