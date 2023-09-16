//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "IdlerComponent.h"

UIdlerComponent::UIdlerComponent()
{
	Radius=10.f;

	ContactPoints.Add(FVector2D(-1,0));
	ContactPoints.Add(FVector2D(0,1));
	
	PrimaryComponentTick.bCanEverTick=false;
}


