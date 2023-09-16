//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VehicleDebugWidget.generated.h"

/**
 * 
 */
UCLASS()
class MODULARMOVEMENT_API UVehicleDebugWidget : public UUserWidget
{
	GENERATED_BODY()

	public:
	UPROPERTY(BlueprintReadOnly,Category=Debug)
	USceneComponent* OwningWheel;
};
