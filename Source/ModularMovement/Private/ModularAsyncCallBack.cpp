//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularAsyncCallBack.h"

#include "ModularMovementComponent.h"


void FModularAsyncCallBack::OnPreSimulate_Internal()
{
	using namespace Chaos;

	const FAsyncPhysicsInput* Input = GetConsumerInput_Internal();

	if(Input == nullptr)
		return;

	UWorld* World = Input->World.Get();

	if(World == nullptr)
		return;

	const TArray<TWeakObjectPtr<UModularMovementComponent>>& ModularMovementComponents = Input->Components;

	for(const TWeakObjectPtr<UModularMovementComponent> MovementComponent : ModularMovementComponents)
	{
		if(!MovementComponent.IsValid())
			continue;
		
		MovementComponent->PhysicsCallBack(GetDeltaTime_Internal());
	}
}

void FModularAsyncCallBack::OnContactModification_Internal(Chaos::FCollisionContactModifier& Modifier)
{
}



