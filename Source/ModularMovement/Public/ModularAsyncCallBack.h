//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Chaos/SimCallbackInput.h"
#include "Chaos/SimCallbackObject.h"
#include "UObject/NoExportTypes.h"


// Data that will be used inside the physics thread
struct FAsyncPhysicsInput : public Chaos::FSimCallbackInput
{
	TArray<TWeakObjectPtr<class UModularMovementComponent>> Components;
	TWeakObjectPtr<UWorld> World;
	bool DebugMessages;

	// Required method
	void Reset()
	{
		Components.Reset();
		World.Reset();
	}
};

// Output from the physics thread
struct FAsyncPhysicsOutput : public Chaos::FSimCallbackOutput
{
	// Required method
	void Reset()
	{
	}
};


class FModularAsyncCallBack : public Chaos::TSimCallbackObject<FAsyncPhysicsInput, FAsyncPhysicsOutput> {
	// This is the function in which we will do our physics STUFF!
	virtual void OnPreSimulate_Internal() override;

	// Not used.
	virtual void OnContactModification_Internal(Chaos::FCollisionContactModifier& Modifier) override;
};
